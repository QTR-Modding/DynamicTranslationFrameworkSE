#pragma once
#include <unordered_map>
#include <shared_mutex>

namespace DynamicTranslationSE {
    using DynamicTranslationFunc = const wchar_t*(__cdecl*)(std::string_view);
    using PapyrusScriptID = std::pair<RE::FormID, std::string>; // formID, editorID

    struct Provider {
        HMODULE hmod{};
        DynamicTranslationFunc native{};
        PapyrusScriptID scriptID{};
    };

    inline std::shared_mutex gProvMutex;
    inline std::unordered_map<std::string, Provider> gProvidersByKey;

    std::wstring InvokeProvider(const Provider& prov, const std::string& a_key);

    bool InstallBindings(RE::BSScript::IVirtualMachine* vm);
}