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