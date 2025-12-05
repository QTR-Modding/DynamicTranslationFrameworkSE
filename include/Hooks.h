#pragma once

namespace Hooks {
    bool Install();

    using TranslateFn = void (*)(RE::GFxTranslator*, RE::GFxTranslator::TranslateInfo*);
    inline TranslateFn g_OrigTranslateAny = nullptr;
    inline void** g_TranslatorVTable = nullptr;

    void Translate_Hook(RE::GFxTranslator* a_this, RE::GFxTranslator::TranslateInfo* a_info);

    // Install the GFx translator vtable hook. Safe to call once at DataLoaded.
    bool InstallTranslatorVtableHook();
}