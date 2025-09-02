#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include "ShowGlitter.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(GameObject) {
    $override
    void customSetup() {
        GameObject::customSetup();

        if (m_isTrigger) {
            m_activateTriggerInEditor = true;
        }
    }
};

class $modify(EffectGameObject) {
    $override
    void triggerObject(GJBaseGameLayer* gameLayer, int p1, gd::vector<int> const* p2) {
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
