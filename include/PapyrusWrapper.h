#pragma once

using CallBack = RE::BSScript::IStackCallbackFunctor;

class CallbackImpl : public CallBack {
public:
    RE::BSFixedString result;

    void operator()(RE::BSScript::Variable a_result) override {
        logger::info("PapyrusWrapper callback invoked.");
        if (a_result.IsString()) {
            logger::info("PapyrusWrapper received string result: {}", a_result.GetString());
            result = a_result.GetString();
        } else {
            logger::warn("PapyrusWrapper callback received non-string result.");
        }
    }

    void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}
};

class PapyrusWrapper : public REX::Singleton<PapyrusWrapper> {
    template <class... Args>
    bool CallFunction(std::string_view functionClass, std::string_view function, RE::BSTSmartPointer<CallBack> callback,
                      Args&&... a_args) {
        auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
        if (!vm) {
            logger::error("PapyrusWrapper: VirtualMachine::GetSingleton() returned null");
            return false;
        }

        logger::info("PapyrusWrapper: VM pointer = {:016X}", reinterpret_cast<std::uintptr_t>(vm));

        auto args = RE::MakeFunctionArguments(std::move(a_args)...);
        const bool ok =
            vm->DispatchStaticCall(RE::BSFixedString{functionClass}, RE::BSFixedString{function}, args, callback);

        delete args;

        return ok;
    }

    static std::wstring Utf8ToWide(const char* utf8) {
        if (!utf8 || *utf8 == '\0') {
            return {};
        }
        const int required = ::MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
        if (required <= 0) {
            return {};
        }
        std::wstring wide(static_cast<std::size_t>(required - 1), L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide.data(), required);
        return wide;
    }

public:
    std::wstring GetDynamicTranslation(std::string_view functionClass, std::string_view functionName, RE::TESForm* item,
                                       RE::TESForm* owner) {
        auto callback = RE::make_smart<CallbackImpl>();


        if (!CallFunction(functionClass, functionName, callback, item, owner)) {
            logger::error("Failed to dispatch Papyrus function {}.{}", functionClass, functionName);
            return {};
        }

        logger::info("PapyrusWrapper result in callback: '{}'", callback->result.c_str());
        return Utf8ToWide(callback->result.c_str());
    }
};