#pragma once
#include <unordered_map>
#include <rapidjson/document.h>
#include "boost/pfr/core.hpp"
#include "CLibUtilsQTR/PresetHelpers/Config.hpp"
#include "DynamicTranslationSE.h"

namespace DynamicTranslationSE {
    struct ConfigEntryBlock {
        Presets::Field<std::vector<std::string>, rapidjson::Value> strings{"strings"};
        Presets::Field<std::string, rapidjson::Value> dll{"skse"};
        Presets::Field<std::string, rapidjson::Value> papyrus{"papyrus"};

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
        static DynamicTranslationFunc ResolveDLLFunction(HMODULE hmod, const std::string& funcName);
        static void ProcessConfigEntry(const ConfigEntryBlock& entry, const std::string& filePath);
    };
}