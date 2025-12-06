#include "DynamicTranslationSE.h"
#include "PapyrusWrapper.h"
#include <coroutine>

struct FireAndForget {
    struct promise_type {
        static FireAndForget get_return_object() noexcept { return {}; }

        // Start running immediately when the function is called
        static std::suspend_never initial_suspend() noexcept { return {}; }

        // When the coroutine finishes, destroy it immediately
        static std::suspend_never final_suspend() noexcept { return {}; }

        static void return_void() noexcept {
        }

        static void unhandled_exception() noexcept {
            try {
                throw;
            } catch (const std::exception& e) {
                logger::error("FireAndForget coroutine error: {}", e.what());
            } catch (...) {
                logger::error("FireAndForget coroutine error: unknown exception");
            }
        }
    };
};

namespace {
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

    FireAndForget RunPapyrusTranslationAsync(const std::string keyUtf8, const std::string functionClass,
                                             const std::string functionName,
                                             RE::TESForm* item, RE::TESForm* owner) {
        // Ask PapyrusWrapper for the Awaitable
        auto awaitable = PapyrusWrapper::GetSingleton()->
            GetDynamicTranslation(functionClass, functionName, item, owner);

        // Suspend here until the VM finishes and the callback resumes us
        RE::BSScript::Variable result = co_await awaitable;

        // When we get here, Papyrus has returned
        if (!result.IsString()) {
            logger::warn("RunPapyrusTranslationAsync: result for key '{}' is not a string", keyUtf8);
            co_return;
        }

        if (auto result_str = result.GetString(); result_str.empty()) {
            logger::warn("RunPapyrusTranslationAsync: result for key '{}' is an empty string", keyUtf8);
        } else if (auto translator = GetTranslator()) {
            logger::info("RunPapyrusTranslationAsync: got Papyrus result '{}' for key '{}'", result_str, keyUtf8);
            papyrusResults[keyUtf8] = result.GetString();
            
            std::wstring key_utf16 = SKSE::stl::utf8_to_utf16("$" + keyUtf8).value_or(L""s);
            RE::GFxWStringBuffer a_result_dummy;

            RE::GFxTranslator::TranslateInfo translateInfo;
            translateInfo.key = key_utf16.c_str();
            translateInfo.result = std::addressof(a_result_dummy);

            translator->Translate(std::addressof(translateInfo));
            translator->Release();
        } else {
            logger::error(
                "RunPapyrusTranslationAsync: could not get GFxTranslator to apply Papyrus result for key '{}'",
                keyUtf8);
        }

        co_return;
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
            if (const auto it = papyrusResults.find(a_key); it != papyrusResults.end()) {
                // Use cached result from previous async call
                logger::info("DynamicTranslationFrameworkSE: using cached Papyrus result {} for key '{}'",
                             it->second, a_key);
                std::wstring result = Utf8ToWide(it->second.c_str());
                return result;
            }
            if (!prov.papyrusClass.empty() && !prov.papyrusFunc.empty()) {
                RunPapyrusTranslationAsync(a_key, prov.papyrusClass, prov.papyrusFunc, item, owner);
                return {}; // Return empty immediately; result will be handled asynchronously
            }
        } catch (...) {
            logger::error("DynamicTranslationFrameworkSE: provider invoke failed");
        }
        return std::wstring{};
    }
}