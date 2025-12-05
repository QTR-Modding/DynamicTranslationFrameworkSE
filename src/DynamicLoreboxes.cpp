#include "DynamicLoreboxes.h"

namespace DynamicLoreboxes {
    std::wstring InvokeProvider(const Provider& prov, RE::TESForm* item, RE::TESForm* owner) {
        try {
            if (prov.native) {
                const auto sv = prov.native(item, owner);
                const std::wstring result(sv);
                return result;
            }
            if (prov.papyrusForm && !prov.papyrusFunc.empty()) {
                // TODO: Papyrus call bridge (client-side optional)
                return std::wstring{};
            }
        } catch (...) {
            logger::error("DynamicLoreboxes: provider invoke failed");
        }
        return std::wstring{};
    }
}