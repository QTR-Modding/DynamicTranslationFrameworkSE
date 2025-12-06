#pragma once
#include "DynamicTranslationSE.h"
#include "Settings.h"
#include "CLibUtilsQTR/Papyrus.hpp"

class PapyrusWrapper : public REX::Singleton<PapyrusWrapper> {
public:
    static RE::BSScript::IVirtualMachine::Awaitable GetDynamicTranslation(
        const DynamicTranslationSE::PapyrusScriptID& scriptID,
        const std::string& a_key) {
        if (const auto a_form = RE::TESForm::LookupByID(scriptID.first)) {
            if (const auto vm = Papyrus::VM::GetSingleton()) {
                auto lock = RE::BSSpinLockGuard(vm->attachedScriptsLock);
                if (const auto a_script = Papyrus::GetAttachedScript(scriptID.second, a_form)) {
                    if (auto obj = static_cast<RE::BSTSmartPointer<RE::BSScript::Object>>(a_script->get())) {
                        using namespace DynamicTranslationFrameworkSE;
                        const auto vmargs = RE::MakeFunctionArguments(static_cast<std::string>(a_key));
                        auto a_awaitable = vm->ADispatchMethodCall(obj, api_function_name, vmargs);
                        delete vmargs;
                        logger::info("GetDynamicTranslation: dispatched method call to script '{}'",
                                     scriptID.second);
                        return a_awaitable;
                    }
                }
            }
        }
        return {};
    }
};