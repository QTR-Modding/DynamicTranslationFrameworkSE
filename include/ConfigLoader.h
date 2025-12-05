#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <windows.h>
#include <rapidjson/document.h>
#include "boost/pfr/core.hpp"
#include "CLibUtilsQTR/PresetHelpers/Config.hpp"
#include "DynamicLoreboxes.h"

namespace DynamicLoreboxes {
    struct ConfigEntryBlock {
        Presets::Field<std::vector<std::string>, rapidjson::Value> keywords{"keywords"};
        Presets::Field<std::string, rapidjson::Value> dll{"dll"};
        Presets::Field<std::string, rapidjson::Value> papyrus{"papyrus"};
        Presets::Field<std::string, rapidjson::Value> function{"function"};

        void load(rapidjson::Value& a_block) {
            boost::pfr::for_each_field(*this, [&](auto& field) {
                field.load(a_block);
            });
        }
    };

    class ConfigLoader {
    public:
        static void Load();

    private:
        static inline std::unordered_map<std::string, HMODULE> dllCache;
        static inline std::mutex dllCacheMutex;

        static HMODULE GetOrLoadDLL(const std::string& dllName);
        static LoreFunc ResolveDLLFunction(HMODULE hmod, const std::string& funcName);
        static void ProcessConfigEntry(const ConfigEntryBlock& entry, const std::string& filePath);
    };
}