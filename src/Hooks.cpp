#include "Hooks.h"
#include "DynamicTranslationSE.h"
#include "Utils.h"

bool Hooks::Install() {
    return InstallTranslatorVtableHook();
}

void Hooks::Translate_Hook(RE::GFxTranslator* a_this, RE::GFxTranslator::TranslateInfo* a_info) {
    auto keyUtf8 = Utils::WideToUtf8(a_info->GetKey());

    // Call original first
    g_OrigTranslateAny(a_this, a_info);

    if (keyUtf8.empty() || keyUtf8[0] != '$') return;
    keyUtf8 = keyUtf8.substr(1);

    // Lookup provider by key
    DynamicTranslationSE::Provider prov{};
    {
        std::shared_lock lk(DynamicTranslationSE::gProvMutex);
        if (const auto it = DynamicTranslationSE::gProvidersByKey.find(keyUtf8);
            it != DynamicTranslationSE::gProvidersByKey.end()) {
            prov = it->second;
        }
    }
    if (!(prov.native || (!prov.papyrusClass.empty() && !prov.papyrusFunc.empty()))) return;

    RE::TESForm* item = nullptr;
    RE::TESForm* owner = nullptr;
    #undef GetObject
    if (const auto* selected = Utils::GetSelectedItemDataInMenu()) {
        if (selected->objDesc) item = selected->objDesc->GetObject();
        owner = Utils::GetOwnerOfItem(selected);
    }

    const auto body = InvokeProvider(prov, item, owner, keyUtf8);
    if (!body.empty()) a_info->SetResult(body.c_str(), body.size());
}

bool Hooks::InstallTranslatorVtableHook() {
    const auto sfm = RE::BSScaleformManager::GetSingleton();
    const auto loader = sfm ? sfm->loader : nullptr;
    const auto tr = loader
                        ? loader->GetState<RE::GFxTranslator>(RE::GFxState::StateType::kTranslator)
                        : RE::GPtr<RE::GFxTranslator>{};

    if (!tr) {
        logger::warn("Translator not available yet; will skip vtable hook.");
        return false;
    }

    // vtable of the active translator instance
    auto** vtbl = *reinterpret_cast<void***>(tr.get());

    // Resolve the vanilla BSScaleformTranslator vtable pointer for comparison
    const REL::Relocation baseVtblRel{RE::BSScaleformTranslator::VTABLE[0]};
    auto** baseVtbl = reinterpret_cast<void**>(baseVtblRel.address());

    // Choose the vtable we will patch: if it's the vanilla class vtable, patch that one; otherwise, patch the
    // instance's
    auto** targetVtbl = vtbl == baseVtbl ? baseVtbl : vtbl;

    // Already patched?
    if (g_TranslatorVTable == targetVtbl && g_OrigTranslateAny) {
        logger::info("Translator vtable already hooked {:p}", static_cast<void*>(targetVtbl));
        return true;
    }

    // Patch vtable slot 0x2 (Translate)
    DWORD oldProt{};
    if (!VirtualProtect(&targetVtbl[2], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProt)) {
        logger::error("VirtualProtect failed while preparing to patch translator vtable.");
        return false;
    }

    g_OrigTranslateAny = reinterpret_cast<TranslateFn>(targetVtbl[2]);
    targetVtbl[2] = reinterpret_cast<void*>(&Translate_Hook);

    DWORD dummy{};
    VirtualProtect(&targetVtbl[2], sizeof(void*), oldProt, &dummy);

    g_TranslatorVTable = targetVtbl;

    logger::info("Installed Translate vtable hook on translator {:p}", static_cast<void*>(targetVtbl));
    return true;
}