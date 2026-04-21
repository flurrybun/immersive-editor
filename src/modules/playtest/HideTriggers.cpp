#include "core/SettingManager.hpp"
#include "events/PlaytestEvent.hpp"
#include "util/Temporary.hpp"
#include "util/ObjectIDs.hpp"

#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(HTGameObject, GameObject) {
    struct Fields {
        bool visibilityLocked = false;
        bool wasVisible = true;
    };

    $register_hooks("hide-triggers");

    $override
    void setVisible(bool visible) {
        if (m_fields->visibilityLocked) {
            m_fields->wasVisible = visible;
        } else {
            GameObject::setVisible(visible);
        }
    }

    void lockVisibility() {
        m_fields->wasVisible = isVisible();

        CCSprite::setVisible(false);

        m_fields->visibilityLocked = true;
    }

    void unlockVisibility() {
        if (!m_fields->visibilityLocked) return;

        m_fields->visibilityLocked = false;
        setVisible(m_fields->wasVisible);
    }
};

class $modify(HTLevelEditorLayer, LevelEditorLayer) {
    $register_hooks("hide-triggers");

    $override
    void updateDebugDraw() {
        // ⏺️ hide guide trigger lines when playtesting
        // ⏺️ hide keyframe lines when playtesting
        // ⏺️ hide trigger boxes when playtesting

        if (m_playbackMode != PlaybackMode::Playing) {
            LevelEditorLayer::updateDebugDraw();
            return;
        }

        ie::withTemporary({
            { &m_cameraGuideTriggers, CCArray::create() },
            { &m_keyframeGroups, CCDictionary::create() },
            { &m_drawTriggerBoxes, false },
        }, [&] {
            LevelEditorLayer::updateDebugDraw();
        });
    }

    bool showInPlaytest(GameObject* object) {
        GameObjectType type = object->m_objectType;

        // particle icons handled in BetterParticleIcons.cpp

        if (
            ie::object::isSpeedPortal(object) ||
            ie::object::isParticle(object) ||
            ie::object::isCheckpoint(object)
        ) return true;

        if (
            ie::object::isTouchToggleBlock(object) ||
            ie::object::isKeyframePoint(object)
        ) return false;

        if (
            type == GameObjectType::Modifier ||
            type == GameObjectType::EnterEffectObject ||
            type == GameObjectType::CollisionObject ||
            type == GameObjectType::Special
        ) return false;

        return true;
    }
};

$on_enable("hide-triggers") {
    auto lel = static_cast<HTLevelEditorLayer*>(ctx.m_lel);

    ctx.addEventListener(PlaytestEvent(), [lel](PlaytestMode mode) {
        for (const auto& object : CCArrayExt<GameObject*>(lel->m_objects)) {
            if (lel->showInPlaytest(object)) continue;

            auto htObject = static_cast<HTGameObject*>(object);

            if (mode.isPlaying()) htObject->lockVisibility();
            else htObject->unlockVisibility();
        }
    });
}
