#pragma once

#include <Geode/loader/Hook.hpp>

void addHookForSetting(const std::string& setting, const std::shared_ptr<geode::Hook>& hook);

#define $bool_setting(name, setting) \
    static bool name = false; \
    $on_mod(DataLoaded) { \
        name = geode::Mod::get()->getSettingValue<bool>(setting); \
        listenForSettingChanges(setting, [](bool value) { \
            name = value; \
        }); \
    }

#define $toggle_hooks(setting) \
    static void onModify(const auto& self) { \
        for (const auto& [key, hook] : self.m_hooks) { \
            addHookForSetting(setting, hook); \
        } \
    }
