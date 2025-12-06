#pragma once

namespace Utils {
    inline std::string WideToUtf8(const wchar_t* w) {
        if (!w) return {};
        const int len = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
        if (len <= 1) return {};
        std::string out(len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, w, -1, out.data(), len, nullptr, nullptr);
        return out;
    }

    template <typename MenuType>
    RE::StandardItemData* GetSelectedItemData() {
        if (const auto ui = RE::UI::GetSingleton()) {
            if (const auto menu = ui->GetMenu<MenuType>()) {
                if (RE::ItemList* a_itemList = menu->GetRuntimeData().itemList) {
                    if (auto* item = a_itemList->GetSelectedItem()) {
                        return &item->data;
                    }
                }
            }
        }
        return nullptr;
    }

    inline RE::StandardItemData* GetSelectedItemDataInMenu(std::string& a_menuOut) {
        if (const auto ui = RE::UI::GetSingleton()) {
            if (ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME)) {
                a_menuOut = RE::InventoryMenu::MENU_NAME;
                return GetSelectedItemData<RE::InventoryMenu>();
            }
            if (ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME)) {
                a_menuOut = RE::ContainerMenu::MENU_NAME;
                return GetSelectedItemData<RE::ContainerMenu>();
            }
            if (ui->IsMenuOpen(RE::BarterMenu::MENU_NAME)) {
                a_menuOut = RE::BarterMenu::MENU_NAME;
                return GetSelectedItemData<RE::BarterMenu>();
            }
        }
        return nullptr;
    }

    inline RE::StandardItemData* GetSelectedItemDataInMenu() {
        std::string menu_name;
        return GetSelectedItemDataInMenu(menu_name);
    }

    inline RE::TESObjectREFR* GetOwnerOfItem(const RE::StandardItemData* a_itemdata) {
        auto& refHandle = a_itemdata->owner;
        if (const auto owner = RE::TESObjectREFR::LookupByHandle(refHandle)) {
            return owner.get();
        }
        if (const auto owner_actor = RE::Actor::LookupByHandle(refHandle)) {
            return owner_actor.get();
        }
        return nullptr;
    }

    struct FireAndForget {
        struct promise_type {
            static FireAndForget get_return_object() noexcept { return {}; }

            // Start running immediately when the function is called
            static std::suspend_never initial_suspend() noexcept { return {}; }

            // When the coroutine finishes, destroy it immediately
            static std::suspend_never final_suspend() noexcept { return {}; }

            static void return_void() noexcept {
            }

            static void unhandled_exception() noexcept {
                try {
                    throw;
                } catch (const std::exception& e) {
                    logger::error("FireAndForget coroutine error: {}", e.what());
                } catch (...) {
                    logger::error("FireAndForget coroutine error: unknown exception");
                }
            }
        };
    };
}