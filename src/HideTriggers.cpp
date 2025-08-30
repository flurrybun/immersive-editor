#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include "misc/PlaytestEvent.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(HTGameObject, GameObject) {
    struct Fields {
        bool visibilityLocked = false;
        bool wasVisible = true;
    };

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

class $modify(LevelEditorLayer) {
    struct Fields {
        PlaytestEventListener playtestListener;
    };

    $override
    bool init(GJGameLevel* p0, bool p1) {
        // ⏺️ hide triggers and other editor-only objects when playtesting

        if (!LevelEditorLayer::init(p0, p1)) return false;

        m_fields->playtestListener.bind([&](PlaytestEvent* event) {
            for (const auto& object : CCArrayExt<GameObject*>(m_objects)) {
                if (showInPlaytest(object)) continue;

                auto htObject = static_cast<HTGameObject*>(object);

                if (event->isPlaying()) htObject->lockVisibility();
                else htObject->unlockVisibility();
            }

            return ListenerResult::Propagate;
        });

        return true;
    }

    bool showInPlaytest(GameObject* object) {
        int id = object->m_objectID;
        GameObjectType type = object->m_objectType;

        bool isSpeedPortal = (id >= 200 && id <= 203) || id == 1334;
        if (isSpeedPortal) return true;

        bool isParticle = id == 1586 || id == 1700 || id == 2065;
        if (isParticle) return false;

        bool isToggleBlock = id == 3643;
        if (isToggleBlock) return false;

        if (type == GameObjectType::Modifier) return false;
        if (type == GameObjectType::EnterEffectObject) return false;
        if (type == GameObjectType::CollisionObject) return false;
        if (type == GameObjectType::Special) return false;

        return true;
    }

    $override
    void updateDebugDraw() {
        // ⏺️ hide guide trigger lines when playtesting

        if (m_playbackMode != PlaybackMode::Playing) {
            LevelEditorLayer::updateDebugDraw();
            return;
        }

        CCArray* prevCGT = m_cameraGuideTriggers;
        m_cameraGuideTriggers = CCArray::create();

        LevelEditorLayer::updateDebugDraw();

        m_cameraGuideTriggers = prevCGT;
    }
};
