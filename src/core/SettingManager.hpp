#pragma once

#include <Geode/loader/Hook.hpp>
#include <Geode/utils/VMTHookManager.hpp>

namespace ie {
    class ModuleContext {
        std::string m_name;
        geode::ListenerHandle m_disableListener;

        std::vector<geode::ListenerHandle*> m_listeners;
        geode::utils::StringMap<geode::Function<void(bool)>> m_toggleVirtuals;

    public:
        LevelEditorLayer* const m_lel;

        ModuleContext(LevelEditorLayer* lel, std::string name);

        ModuleContext(const ModuleContext&) = delete;
        ModuleContext& operator=(const ModuleContext&) = delete;

        template <class Event, class Callback>
        void addEventListener(const Event& event, Callback&& callback) {
            geode::ListenerHandle* listener = m_lel->addEventListener(event, std::forward<Callback>(callback));
            m_listeners.push_back(listener);
        }
        template <auto Function, class Class>
        void addVirtualHook(Class* instance, const std::string& name) {
            auto it = m_toggleVirtuals.find(name);

            if (it != m_toggleVirtuals.end()) {
                it->second(true);
                return;
            }

            auto res = geode::VMTHookManager::get().addHook<Function, Class>(instance, name);

            if (!res) {
                geode::log::warn("Failed to hook virtual function '{}'", name);
                return;
            }

            m_toggleVirtuals[name] = [instance, name](bool enable) {
                auto res = enable
                    ? geode::VMTHookManager::get().forceEnableFunction<Function, Class>(instance)
                    : geode::VMTHookManager::get().forceDisableFunction<Function, Class>(instance);

                if (!res) {
                    geode::log::warn("Failed to {} virtual function '{}'", enable ? "enable" : "disable", name);
                }
            };
        }
    };

    class EnableModuleEvent : public geode::Event<EnableModuleEvent, bool(ModuleContext&), std::string> {
    public:
        using Event::Event;
    };

    class DisableModuleEvent : public geode::Event<DisableModuleEvent, bool(ModuleContext&), std::string> {
    public:
        using Event::Event;
    };

    void addHookForSetting(const std::string& setting, const std::shared_ptr<geode::Hook>& hook);
}

#define $bind_setting(name, setting) \
    static bool name = false; \
    $on_mod(DataLoaded) { \
        name = geode::Mod::get()->getSettingValue<bool>(setting); \
        listenForSettingChanges<bool>(setting, [](bool value) { \
            name = value; \
        }); \
    }

#define $register_hooks(setting) \
    static void onModify(const auto& self) { \
        for (const auto& [key, hook] : self.m_hooks) { \
            ie::addHookForSetting(setting, hook); \
        } \
    }

// mostly stolen from geode's $execute macro

#define $enable_base(setting, event) \
    namespace { namespace GEODE_UNITY_NS_ID { \
        struct GEODE_CONCAT(EnableUnique, __LINE__) {}; \
        void GEODE_CONCAT(enableFuncI, __LINE__)(ie::ModuleContext&); \
        template<class> \
        void GEODE_CONCAT(enableFunc1, __LINE__)() { \
            ie::event(setting).listen([](ie::ModuleContext& ctx) { \
                GEODE_CONCAT(enableFuncI, __LINE__)(ctx); \
            }).leak(); \
        } \
        static inline auto GEODE_CONCAT(EnableExec, __LINE__) = \
            (GEODE_CONCAT(enableFunc1, __LINE__)<GEODE_CONCAT(EnableUnique, __LINE__)>(), 0); \
    }} \
    void GEODE_UNITY_NS_ID::GEODE_CONCAT(enableFuncI, __LINE__)(ie::ModuleContext& ctx)

#define $on_enable(setting) $enable_base(setting, EnableModuleEvent)
#define $on_disable(setting) $enable_base(setting, DisableModuleEvent)
