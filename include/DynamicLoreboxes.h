#pragma once
#include <unordered_map>
#include <shared_mutex>
#include <windows.h>

namespace DynamicLoreboxes {
    using LoreFunc = const wchar_t*(__cdecl*)(RE::TESForm* item, RE::TESForm* owner);

    struct Provider {
        // DLL
        HMODULE hmod{};
        LoreFunc native{};
        // Papyrus
        std::string papyrusClass{};
        std::string papyrusFunc;
    };

    // Registry keyed by translator key (UTF-8). Config loader will populate this.
    inline std::shared_mutex gProvMutex;
    inline std::unordered_map<std::string, Provider> gProvidersByKey; // key is keyword form

    std::wstring InvokeProvider(const Provider& prov, RE::TESForm* item, RE::TESForm* owner);
}