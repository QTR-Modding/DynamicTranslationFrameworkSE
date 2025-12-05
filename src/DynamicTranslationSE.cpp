#include "DynamicTranslationSE.h"
#include "PapyrusWrapper.h"

namespace DynamicTranslationSE {
    std::wstring InvokeProvider(const Provider& prov, RE::TESForm* item, RE::TESForm* owner) {
        try {
            if (prov.native) {
                const auto sv = prov.native(item, owner);
                const std::wstring result(sv);
                return result;
            }
            if (!prov.papyrusClass.empty() && !prov.papyrusFunc.empty()) {
                return PapyrusWrapper::GetSingleton()->GetDynamicLoreBoxText(prov.papyrusClass, prov.papyrusFunc, item, owner);
            }
        } catch (...) {
            logger::error("DynamicLoreboxes: provider invoke failed");
        }
        return std::wstring{};
    }
}