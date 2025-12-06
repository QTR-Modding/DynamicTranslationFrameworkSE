#include "ConfigLoader.h"
#include "Settings.h"
#include <rapidjson/error/en.h>

namespace DynamicTranslationSE {
    HMODULE ConfigLoader::GetOrLoadDLL(const std::string& dllName) {
        std::lock_guard lock(dllCacheMutex);

        const auto it = dllCache.find(dllName);
        if (it != dllCache.end()) {
            return it->second;
        }

        // Convert to wide string for LoadLibraryW
        const int wideLen = MultiByteToWideChar(CP_UTF8, 0, dllName.c_str(), -1, nullptr, 0);
        if (wideLen <= 0) {
            logger::error("ConfigLoader: Failed to convert DLL name to wide string: {}", dllName);
            return nullptr;
        }

        std::wstring wideName(wideLen, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, dllName.c_str(), -1, wideName.data(), wideLen);

        const HMODULE hmod = LoadLibraryW(wideName.c_str());
        if (!hmod) {
            logger::error("ConfigLoader: Failed to load DLL: {} (error: {})", dllName, GetLastError());
            return nullptr;
        }

        dllCache[dllName] = hmod;
        logger::info("ConfigLoader: Loaded DLL: {}", dllName);
        return hmod;
    }

    DynamicTranslationFunc ConfigLoader::ResolveDLLFunction(HMODULE hmod, const std::string& funcName) {
        if (!hmod || funcName.empty()) {
            return nullptr;
        }

        const auto proc = GetProcAddress(hmod, funcName.c_str());
        if (!proc) {
            logger::error("ConfigLoader: Failed to resolve function '{}' (error: {})", funcName, GetLastError());
            return nullptr;
        }

        return reinterpret_cast<DynamicTranslationFunc>(proc);
    }

    void ConfigLoader::ProcessConfigEntry(const ConfigEntryBlock& entry, const std::string& filePath) {
        const auto& strings = entry.strings.get();
        const auto& dllName = entry.dll.get();
        const auto& editorId = entry.papyrus.get();
        const auto form = RE::TESForm::LookupByEditorID(editorId.c_str());
        const auto formID = form ? form->GetFormID() : 0;

        const bool hasDll = !dllName.empty();
        const bool hasPapyrus = formID > 0 && !editorId.empty();

        if (!hasDll && !hasPapyrus) {
            logger::warn("ConfigLoader: Entry in '{}' has neither 'dll' nor 'papyrus', skipping", filePath);
            return;
        }

        // Load DLL and resolve fixed function name if native
        HMODULE hmod = nullptr;
        DynamicTranslationFunc nativeFunc = nullptr;
        if (hasDll) {
            hmod = GetOrLoadDLL(dllName);
            if (hmod) {
                nativeFunc = ResolveDLLFunction(hmod, "OnDynamicTranslationRequest");
                if (!nativeFunc) {
                    logger::error(
                        "ConfigLoader: Failed to resolve native function 'OnDynamicTranslationRequest' from DLL '{}' in '{}'",
                        dllName, filePath);
                    if (!hasPapyrus) {
                        return;
                    }
                }
            } else {
                if (!hasPapyrus) {
                    return;
                }
            }
        }

        // Process strings
        for (const auto& a_str : strings) {
            if (a_str.empty()) {
                logger::warn("ConfigLoader: Empty translation string in entry from '{}', skipping", filePath);
                continue;
            }

            Provider prov{};
            prov.hmod = hmod;
            prov.native = nativeFunc;
            if (hasPapyrus) {
                prov.scriptID = {formID, editorId};
            }

            {
                std::unique_lock lock(gProvMutex);
                gProvidersByKey[a_str] = std::move(prov);
            }

            logger::info("ConfigLoader: Registered provider for translation string '{}'", a_str);
        }
    }

    void ConfigLoader::Load() {
        using namespace DynamicTranslationFrameworkSE;
        if (!std::filesystem::exists(kConfigFolder)) {
            logger::info("ConfigLoader: Config folder does not exist: {}", kConfigFolder);
            return;
        }

        logger::info("ConfigLoader: Loading configurations from {}", kConfigFolder);

        {
            std::unique_lock lock(gProvMutex);
            gProvidersByKey.clear();
        }

        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(kConfigFolder, ec)) {
            if (ec) {
                logger::error("ConfigLoader: Error iterating directory: {}", ec.message());
                break;
            }

            if (!entry.is_regular_file()) {
                continue;
            }

            const auto& path = entry.path();
            if (path.extension() != ".json") {
                continue;
            }

            const std::string filePath = path.string();
            logger::info("ConfigLoader: Processing file: {}", filePath);

            std::ifstream ifs(path);
            if (!ifs.is_open()) {
                logger::error("ConfigLoader: Failed to open file: {}", filePath);
                continue;
            }

            std::string jsonStr((std::istreambuf_iterator(ifs)), std::istreambuf_iterator<char>());
            ifs.close();

            rapidjson::Document doc;
            doc.Parse(jsonStr.c_str());

            if (doc.HasParseError()) {
                logger::error("ConfigLoader: JSON parse error in '{}' at offset {}: {}",
                              filePath, doc.GetErrorOffset(), rapidjson::GetParseError_En(doc.GetParseError()));
                continue;
            }

            if (doc.IsArray()) {
                for (auto& item : doc.GetArray()) {
                    if (!item.IsObject()) {
                        logger::warn("ConfigLoader: Array item in '{}' is not an object, skipping", filePath);
                        continue;
                    }
                    ConfigEntryBlock block;
                    block.load(item);
                    ProcessConfigEntry(block, filePath);
                }
            } else if (doc.IsObject()) {
                ConfigEntryBlock block;
                block.load(doc);
                ProcessConfigEntry(block, filePath);
            } else {
                logger::error("ConfigLoader: Root element in '{}' is neither object nor array", filePath);
            }
        }

        logger::info("ConfigLoader: Configuration loading complete");
    }
}