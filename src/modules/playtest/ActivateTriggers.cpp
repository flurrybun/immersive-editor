#include "core/SettingManager.hpp"
#include "modules/level-effects/ShowGlitter.hpp"
#include "util/Editor.hpp"
#include "util/ObjectIDs.hpp"

#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/utils/VMTHookManager.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(GameObject) {
    $register_hooks("activate-triggers");

    $override
    void customSetup() {
        // ⏺️ prevent some triggers not activating in editor

        GameObject::customSetup();

        if (m_isTrigger) {
            m_activateTriggerInEditor = true;
        }
    }
};

class $modify(EffectGameObject) {
    $register_hooks("activate-triggers");

    $override
    void triggerObject(GJBaseGameLayer* gameLayer, int p1, gd::vector<int> const* p2) {
        // ⏺️ activate ghost effect triggers
        // ⏺️ activate bg effect triggers

        if (!ie::inEditor()) {
            EffectGameObject::triggerObject(gameLayer, p1, p2);
            return;
        }

        switch (m_objectID) {
            case ie::object::EnableGhostTrailTrigger:
                gameLayer->m_player1->toggleGhostEffect(GhostType::Enabled);
                if (gameLayer->m_gameState.m_isDualMode) {
                    gameLayer->m_player2->toggleGhostEffect(GhostType::Enabled);
                }
                break;
            case ie::object::DisableGhostTrailTrigger:
                gameLayer->m_player1->toggleGhostEffect(GhostType::Disabled);
                if (gameLayer->m_gameState.m_isDualMode) {
                    gameLayer->m_player2->toggleGhostEffect(GhostType::Disabled);
                }
                break;
            case ie::object::EnableBGEffectTrigger:
                toggleBGEffectVisibility(true);
                break;
            case ie::object::DisableBGEffectTrigger:
                toggleBGEffectVisibility(false);
                break;
            default:
                EffectGameObject::triggerObject(gameLayer, p1, p2);
                break;
        }
    }
};

class $modify(ATLevelEditorLayer, LevelEditorLayer) {
    $register_hooks("activate-triggers");

    $override
    bool init(GJGameLevel* p0, bool p1) {
        // ⏺️ show hide ground/mg options in option triggers

        if (!LevelEditorLayer::init(p0, p1)) return false;

        (void)VMTHookManager::get().addHook<
            ResolveC<ATLevelEditorLayer>::func(&ATLevelEditorLayer::toggleGroundVisibility)
        >(this, "LevelEditorLayer::toggleGroundVisibility");

        (void)VMTHookManager::get().addHook<
            ResolveC<ATLevelEditorLayer>::func(&ATLevelEditorLayer::toggleMGVisibility)
        >(this, "LevelEditorLayer::toggleMGVisibility");

        return true;
    }

    void toggleGroundVisibility(bool visible) {
        m_groundLayer->toggleVisible02(visible);
        m_groundLayer2->toggleVisible02(visible);
    }

    void toggleMGVisibility(bool visible) {
        if (!m_middleground) return;
        m_middleground->toggleVisible02(visible);
    }
};
