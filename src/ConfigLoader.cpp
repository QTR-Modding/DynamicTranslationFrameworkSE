#include "ConfigLoader.h"
#include "logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <rapidjson/error/en.h>

namespace DynamicLoreboxes {

    namespace {
        constexpr std::string_view kConfigFolder = R"(Data\SKSE\Plugins\DynamicLoreBoxes)";

        bool StartsWithHex(const std::string& str) {
            return str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X');
        }

        bool IsDecimalNumber(const std::string& str) {
            if (str.empty()) return false;
            for (char c : str) {
                if (!std::isdigit(static_cast<unsigned char>(c))) return false;
            }
            return true;
        }

        RE::FormID ParseFormID(const std::string& str) {
            RE::FormID formId = 0;
            if (StartsWithHex(str)) {
                std::stringstream ss;
                ss << std::hex << str;
                ss >> formId;
            } else if (IsDecimalNumber(str)) {
                formId = static_cast<RE::FormID>(std::stoul(str));
            }
            return formId;
        }
    }

    HMODULE ConfigLoader::GetOrLoadDLL(const std::string& dllName) {
        std::lock_guard<std::mutex> lock(dllCacheMutex);

        auto it = dllCache.find(dllName);
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

        HMODULE hmod = LoadLibraryW(wideName.c_str());
        if (!hmod) {
            logger::error("ConfigLoader: Failed to load DLL: {} (error: {})", dllName, GetLastError());
            return nullptr;
        }

        dllCache[dllName] = hmod;
        logger::info("ConfigLoader: Loaded DLL: {}", dllName);
        return hmod;
    }

    LoreFunc ConfigLoader::ResolveDLLFunction(HMODULE hmod, const std::string& funcName) {
        if (!hmod || funcName.empty()) {
            return nullptr;
        }

        auto proc = GetProcAddress(hmod, funcName.c_str());
        if (!proc) {
            logger::error("ConfigLoader: Failed to resolve function '{}' (error: {})", funcName, GetLastError());
            return nullptr;
        }

        return reinterpret_cast<LoreFunc>(proc);
    }

    RE::BGSKeyword* ConfigLoader::ResolveKeyword(const std::string& keywordStr) {
        if (keywordStr.empty()) {
            return nullptr;
        }

        // Try hex or decimal form ID first
        if (StartsWithHex(keywordStr) || IsDecimalNumber(keywordStr)) {
            RE::FormID formId = ParseFormID(keywordStr);
            if (formId > 0) {
                if (auto* form = RE::TESForm::LookupByID<RE::BGSKeyword>(formId)) {
                    return form;
                }
            }
        }

        // Try the full FormReader resolution (handles plugin~formid and editor IDs)
        if (const auto formId = FormReader::GetFormEditorIDFromString(keywordStr); formId > 0) {
            if (auto* form = RE::TESForm::LookupByID<RE::BGSKeyword>(formId)) {
                return form;
            }
        }

        // Final fallback: direct editor ID lookup
        if (auto* form = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(keywordStr)) {
            return form;
        }

        return nullptr;
    }

    void ConfigLoader::ProcessConfigEntry(const ConfigEntryBlock& entry, const std::string& filePath) {
        const auto& keywords = entry.keywords.get();
        const auto& dllName = entry.dll.get();
        const auto& papyrusClass = entry.papyrus.get();
        const auto& funcName = entry.function.get();

        const bool hasDll = !dllName.empty();
        const bool hasPapyrus = !papyrusClass.empty();
        const bool hasFunction = !funcName.empty();

        // Validate: function is required if dll or papyrus is specified
        if ((hasDll || hasPapyrus) && !hasFunction) {
            logger::error("ConfigLoader: Entry in '{}' has 'dll' or 'papyrus' but no 'function'", filePath);
            return;
        }

        // Validate: at least one provider type must be specified
        if (!hasDll && !hasPapyrus) {
            logger::warn("ConfigLoader: Entry in '{}' has no 'dll' or 'papyrus' specified, skipping", filePath);
            return;
        }

        // Load DLL and resolve function if native
        HMODULE hmod = nullptr;
        LoreFunc nativeFunc = nullptr;
        if (hasDll) {
            hmod = GetOrLoadDLL(dllName);
            if (hmod) {
                nativeFunc = ResolveDLLFunction(hmod, funcName);
                if (!nativeFunc) {
                    logger::error("ConfigLoader: Failed to resolve native function '{}' from DLL '{}' in '{}'",
                        funcName, dllName, filePath);
                    // Continue anyway if papyrus is also specified
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

        // Process keywords
        for (const auto& kwStr : keywords) {
            auto* keyword = ResolveKeyword(kwStr);
            if (!keyword) {
                logger::warn("ConfigLoader: Failed to resolve keyword '{}' in '{}'", kwStr, filePath);
                continue;
            }

            // Get the keyword's editor ID or form name for the registry key
            std::string keyName;
            const char* editorId = keyword->GetFormEditorID();
            if (editorId && editorId[0] != '\0') {
                keyName = editorId;
            } else {
                // Fallback to hex form ID
                std::stringstream ss;
                ss << "0x" << std::hex << keyword->GetFormID();
                keyName = ss.str();
            }

            // Create and populate the Provider
            Provider prov{};
            prov.hmod = hmod;
            prov.native = nativeFunc;

            if (hasPapyrus) {
                // For Papyrus, we store the class name in papyrusForm (as a placeholder)
                // and the function name in papyrusFunc
                // Note: Full Papyrus bridging would require looking up the script form
                prov.papyrusFunc = funcName;
                // papyrusForm would need proper resolution in a real Papyrus bridge
                // For now, we leave it as nullptr (TODO marker)
            }

            // Register the provider
            {
                std::unique_lock<std::shared_mutex> lock(gProvMutex);
                gProvidersByKey[keyName] = std::move(prov);
            }

            logger::info("ConfigLoader: Registered provider for keyword '{}'", keyName);
        }
    }

    void ConfigLoader::Load() {
        if (!std::filesystem::exists(kConfigFolder)) {
            logger::info("ConfigLoader: Config folder does not exist: {}", kConfigFolder);
            return;
        }

        logger::info("ConfigLoader: Loading configurations from {}", kConfigFolder);

        // Clear existing registry for reload
        {
            std::unique_lock<std::shared_mutex> lock(gProvMutex);
            gProvidersByKey.clear();
        }

        // Iterate through the folder and subfolders
        std::error_code ec;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(kConfigFolder, ec)) {
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

            // Read the file
            std::ifstream ifs(path);
            if (!ifs.is_open()) {
                logger::error("ConfigLoader: Failed to open file: {}", filePath);
                continue;
            }

            std::string jsonStr((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            ifs.close();

            // Parse JSON
            rapidjson::Document doc;
            doc.Parse(jsonStr.c_str());

            if (doc.HasParseError()) {
                logger::error("ConfigLoader: JSON parse error in '{}' at offset {}: {}",
                    filePath, doc.GetErrorOffset(), rapidjson::GetParseError_En(doc.GetParseError()));
                continue;
            }

            // Handle both array and single object formats
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
