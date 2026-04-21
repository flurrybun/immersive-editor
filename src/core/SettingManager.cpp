#include "SettingManager.hpp"

#include <Geode/modify/LevelEditorLayer.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

ie::ModuleContext::ModuleContext(LevelEditorLayer* lel, std::string name)
    : m_lel(lel), m_name(std::move(name)) {
    m_disableListener = DisableModuleEvent(m_name).listen([this](ie::ModuleContext&) {
        for (auto listener : m_listeners) {
            m_lel->removeEventListener(listener);
        }

        m_listeners.clear();

        for (auto& [name, toggle] : m_toggleVirtuals) {
            toggle(false);
        }
    });
}

static StringMap<ie::ModuleContext> g_contexts;

void addContextForSetting(const std::string& setting, LevelEditorLayer* lel) {
    auto [ctx, _] = g_contexts.try_emplace(setting, lel, setting);
    ie::EnableModuleEvent(setting).send(ctx->second);
}

void removeContextForSetting(const std::string& setting) {
    auto it = g_contexts.find(setting);
    if (it == g_contexts.end()) return;

    ie::DisableModuleEvent(setting).send(it->second);
    g_contexts.erase(it);
}

class $modify(LevelEditorLayer) {
    struct Fields {
        ~Fields() {
            g_contexts.clear();
        }
    };

    $override
    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        // ensure fields dtor is called
        m_fields.self();

        for (const auto& key : Mod::get()->getSettingKeys()) {
            if (!Mod::get()->getSettingValue<bool>(key)) continue;

            addContextForSetting(key, this);
        }

        return true;
    }
};

std::unordered_map<std::string, std::vector<std::shared_ptr<Hook>>>& getHooks() {
    static std::unordered_map<std::string, std::vector<std::shared_ptr<Hook>>> s_hooks;
    return s_hooks;
}

void ie::addHookForSetting(const std::string& setting, const std::shared_ptr<Hook>& hook) {
    getHooks()[setting].push_back(hook);
}

$on_game(Loaded) {
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

    listenForAllSettingChanges([](std::string_view key_sv, std::shared_ptr<SettingV3> setting) {
        auto key = std::string(key_sv);
        bool enable = Mod::get()->getSettingValue<bool>(key);
        auto& hooks = getHooks();

        if (auto lel = LevelEditorLayer::get()) {
            if (enable) addContextForSetting(key, lel);
            else removeContextForSetting(key);
        }

        auto it = hooks.find(key);
        if (it == hooks.end()) return;

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
