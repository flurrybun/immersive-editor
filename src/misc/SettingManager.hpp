#pragma once

#include <Geode/loader/Hook.hpp>
#include <ranges>

using namespace geode::prelude;

void addHookForSetting(const std::string& setting, const std::shared_ptr<Hook>& hook);

#define $bool_setting(name, setting) \
    static bool name = false; \
    $on_mod(DataLoaded) { \
        name = Mod::get()->getSettingValue<bool>(setting); \
        listenForSettingChanges(setting, [](bool value) { \
            name = value; \
        }); \
    }

#define $toggle_hooks(setting) \
    static void onModify(const auto& self) { \
        for (const auto& hook : self.m_hooks | std::views::values) { \
            addHookForSetting(setting, hook); \
        } \
    }
