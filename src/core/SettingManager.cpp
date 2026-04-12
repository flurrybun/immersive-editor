#include "SettingManager.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

std::unordered_map<std::string, std::vector<std::shared_ptr<Hook>>>& getHooks() {
    static std::unordered_map<std::string, std::vector<std::shared_ptr<Hook>>> s_hooks;
    return s_hooks;
}

void addHookForSetting(
    const std::string& setting,
    const std::shared_ptr<Hook>& hook
) {
    getHooks()[setting].push_back(hook);
}

$execute {
    for (const auto& [key, hooks] : getHooks()) {
        if (Mod::get()->getSettingValue<bool>(key)) continue;

        for (const auto& hook : hooks) {
            auto res = hook->toggle(false);
            if (res.isOk()) continue;

            log::warn(
                "Failed to disable {} hook for setting '{}'",
                hook->getDisplayName(), key
            );
        }
    }

    listenForAllSettingChanges([](std::string_view key, std::shared_ptr<SettingV3> setting) {
        auto& hooks = getHooks();

        auto it = hooks.find(std::string(key));
        if (it == hooks.end()) return;

        bool enable = Mod::get()->getSettingValue<bool>(key);

        for (const auto& hook : it->second) {
            auto res = hook->toggle(enable);
            if (res.isOk()) continue;

            log::warn(
                "Failed to {} {} hook for setting '{}'",
                enable ? "enable" : "disable", hook->getDisplayName(), key
            );
        }
    });
};
