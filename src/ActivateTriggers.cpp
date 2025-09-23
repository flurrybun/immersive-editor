#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/utils/VMTHookManager.hpp>
#include "ShowGlitter.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(GameObject) {
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
    $override
    void triggerObject(GJBaseGameLayer* gameLayer, int p1, gd::vector<int> const* p2) {
        // ⏺️ activate ghost effect triggers
        // ⏺️ activate bg effect triggers

        if (!LevelEditorLayer::get()) {
            EffectGameObject::triggerObject(gameLayer, p1, p2);
            return;
        }

        switch (m_objectID) {
            case 32:
            case 33: {
                GhostType type = m_objectID == 32 ? GhostType::Enabled : GhostType::Disabled;

                gameLayer->m_player1->toggleGhostEffect(type);
                if (gameLayer->m_gameState.m_isDualMode) {
                    gameLayer->m_player2->toggleGhostEffect(type);
                }
                break;
            }
            case 1818:
            case 1819:
                toggleBGEffectVisibility(m_objectID == 1818);
                break;
            default:
                EffectGameObject::triggerObject(gameLayer, p1, p2);
                break;
        }
    }
};

class $modify(ATLevelEditorLayer, LevelEditorLayer) {
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
