#pragma once
#include <unordered_map>
#include <shared_mutex>

namespace DynamicTranslationSE {
    using DynamicTranslationFunc = const wchar_t*(__cdecl*)(RE::TESForm* item, RE::TESForm* owner);

    struct Provider {
        // DLL
        HMODULE hmod{};
        DynamicTranslationFunc native{};
        // Papyrus
        std::string papyrusClass{};
        std::string papyrusFunc;
    };

    // Registry keyed by translator key (UTF-8). Config loader will populate this.
    inline std::shared_mutex gProvMutex;
    inline std::unordered_map<std::string, Provider> gProvidersByKey; // key is keyword form

    std::wstring InvokeProvider(const Provider& prov, RE::TESForm* item, RE::TESForm* owner, const std::string& a_key);

    bool InstallBindings(RE::BSScript::IVirtualMachine* vm);
}