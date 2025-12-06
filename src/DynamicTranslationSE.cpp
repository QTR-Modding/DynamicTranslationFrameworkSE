#include "DynamicTranslationSE.h"
#include "PapyrusWrapper.h"
#include "Utils.h"

namespace {
    std::shared_mutex papyrusResultsMutex;
    inline std::unordered_map<std::string, std::string> papyrusResults;


    std::wstring Utf8ToWide(const char* utf8) {
        if (!utf8 || *utf8 == '\0') {
            return {};
        }
        const int required = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
        if (required <= 0) {
            return {};
        }
        std::wstring wide(static_cast<std::size_t>(required - 1), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide.data(), required);
        return wide;
    }

    RE::GFxTranslator* GetTranslator() {
        const auto scaleformManager = RE::BSScaleformManager::GetSingleton();
        const auto loader = scaleformManager ? scaleformManager->loader : nullptr;
        const auto translator =
            loader ? loader->GetStateAddRef<RE::GFxTranslator>(RE::GFxState::StateType::kTranslator) : nullptr;
        return translator;
    }

    [[maybe_unused]] bool Translate(const std::wstring& key, RE::GFxWStringBuffer& result) {
        if (const auto translator = GetTranslator()) {
            RE::GFxTranslator::TranslateInfo translateInfo;
            translateInfo.key = key.c_str();
            translateInfo.result = std::addressof(result);
            translator->Translate(std::addressof(translateInfo));
            translator->Release();
            return true;
        }
        return false;
    }

    Utils::FireAndForget RunPapyrusTranslationAsync(const std::string keyUtf8, const std::string& functionClass,
                                                    const std::string& functionName,
                                                    RE::TESForm* item, RE::TESForm* owner) {
        // Ask PapyrusWrapper for the Awaitable
        auto awaitable = PapyrusWrapper::GetSingleton()->
            GetDynamicTranslation(functionClass, functionName, item, owner);

        // Suspend here until the VM finishes and the callback resumes us
        const RE::BSScript::Variable result = co_await awaitable;

        // When we get here, Papyrus has returned
        if (!result.IsString()) {
            logger::warn("RunPapyrusTranslationAsync: result for key '{}' is not a string", keyUtf8);
            co_return;
        }

        if (auto result_str = result.GetString(); !result_str.empty()) {
            logger::info("RunPapyrusTranslationAsync: got Papyrus result '{}' for key '{}'", result_str, keyUtf8);
            std::unique_lock lk(papyrusResultsMutex);
            papyrusResults[keyUtf8] = result_str;
        }
        co_return;
    }

    void DynamicTranslateV1(RE::StaticFunctionTag*, std::string a_key,
                            std::string a_val) { // NOLINT(performance-unnecessary-value-param)
        std::unique_lock lk(papyrusResultsMutex);
        papyrusResults[std::move(a_key)] = std::move(a_val);
    }
}


namespace DynamicTranslationSE {
    std::wstring InvokeProvider(const Provider& prov, RE::TESForm* item, RE::TESForm* owner, const std::string& a_key) {
        try {
            if (prov.native) {
                const auto sv = prov.native(item, owner);
                const std::wstring result(sv);
                return result;
            }
            if (!prov.papyrusClass.empty() && !prov.papyrusFunc.empty()) {
                RunPapyrusTranslationAsync(a_key, prov.papyrusClass, prov.papyrusFunc, item, owner);
            }
            {
                std::shared_lock lk(papyrusResultsMutex);
                if (const auto it = papyrusResults.find(a_key); it != papyrusResults.end()) {
                    // Use cached result from previous async call
                    logger::info("DynamicTranslationFrameworkSE: using cached Papyrus result {} for key '{}'",
                                 it->second, a_key);
                    std::wstring result = Utf8ToWide(it->second.c_str());
                    return result;
                }
            }
        } catch (...) {
            logger::error("DynamicTranslationFrameworkSE: provider invoke failed");
        }
        return std::wstring{};
    }

    bool InstallBindings(RE::BSScript::IVirtualMachine* vm) {
        vm->RegisterFunction("DynamicTranslateV1", "DynamicTranslationFramework", DynamicTranslateV1);
        return true;
    }
}