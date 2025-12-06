#pragma once

class PapyrusWrapper : public REX::Singleton<PapyrusWrapper> {
    template <class... Args>
    RE::BSScript::IVirtualMachine::Awaitable CallFunction(const std::string_view functionClass,
                                                          const std::string_view function,
                                                          Args&&... a_args) {
        auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
        if (!vm) {
            logger::error("PapyrusWrapper: VirtualMachine::GetSingleton() returned null");
            return {};
        }

        logger::info("PapyrusWrapper: VM pointer = {:016X}", reinterpret_cast<std::uintptr_t>(vm));

        auto args = RE::MakeFunctionArguments(std::move(a_args)...);
        auto a_awaitable =
            vm->ADispatchStaticCall(RE::BSFixedString{functionClass}, RE::BSFixedString{function}, args);

        delete args;

        return a_awaitable;
    }

public:
    RE::BSScript::IVirtualMachine::Awaitable GetDynamicTranslation(const std::string_view functionClass,
                                                                   const std::string_view functionName,
                                                                   RE::TESForm* item,
                                                                   RE::TESForm* owner) {
        return CallFunction(functionClass, functionName, item, owner);
    }
};