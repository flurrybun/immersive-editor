#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "UpdateVisibility.hpp"
#include "misc/PlaytestEvent.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(GJBaseGameLayer) {
    $override
    void collisionCheckObjects(PlayerObject* p0, gd::vector<GameObject*>* p1, int p2, float p3) {
        // ⏺️ activate mirror portals in editor

        if (!LevelEditorLayer::get()) {
            GJBaseGameLayer::collisionCheckObjects(p0, p1, p2, p3);
            return;
        }

        m_isEditor = false;
        GJBaseGameLayer::collisionCheckObjects(p0, p1, p2, p3);
        m_isEditor = true;
    }
};

class $modify(SMELevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        PlaytestEventListener playtestListener;
    };

    $override
    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        m_fields->playtestListener.bind([this](PlaytestEvent* event) {
            if (event->isPlaying()) {
                if (
                    m_startPosObject &&
                    m_startPosObject->m_startSettings &&
                    m_startPosObject->m_startSettings->m_mirrorMode
                ) {
                    m_gameState.m_levelFlipping = 1.f;
                }
            } else {
                m_gameState.m_levelFlipping = 0.f;
                m_gameState.m_unkBool11 = false;
                m_gameState.m_unkBool12 = false;
            }

            if (event->isNot()) {
                auto& flipTween = m_gameState.m_tweenActions[7];
                flipTween.m_currentValue = 0.f;
                flipTween.m_finished = true;
            }

            return ListenerResult::Propagate;
        });

        return true;
    }

    void screenFlipObject(GameObject* object) {
        // PlayLayer::screenFlipObject is inlined on 2.2074 android64
        // TY JASMINE FOR DECOMPING THIS ILY ❤️

        auto flip = m_cameraFlip;
        auto flipping = m_gameState.m_levelFlipping;

        float factor = flip == -1.f
            ? 1.f - flipping
            : flipping;

        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        CCPoint objectPos = object->getPosition();

        float xDiff = objectPos.x - m_gameState.m_cameraPosition.x;
        object->setPosition(
            objectPos +
            ccp((winSize.width / m_gameState.m_cameraZoom - (xDiff + xDiff)) * factor, 0.f)
        );

        float angle = std::abs(object->getRotation());
        bool rotated = angle == 90.f || angle == 270.f;

        if ((flip != 1.f && flipping > .5f) || (flip == 1.f && flipping < .5f)) {
            if (!m_gameState.m_unkBool11) return;
            int sign = flip == 1.f ? 1 : -1;

            if (rotated) object->setFlipY(object->m_startFlipY * sign != 0);
            else object->setFlipX(object->m_startFlipX * sign != 0);

            if (static_cast<int>(angle) % 90 != 0) object->setRotation(object->m_startRotationX * sign);
        }
        else {
            if (rotated) object->setFlipY(!object->m_startFlipY);
            else object->setFlipX(!object->m_startFlipX);

            if (static_cast<int>(angle) % 90 != 0) object->setRotation(-object->m_startRotationX);
        }
    }
};

class $modify(GameObject) {
    $override
    void updateStartValues() {
        // ⏺️ fix objects being flipped the wrong direction when stopping playtest during 2nd half of mirror effect

        setFlipX(m_startFlipX);
        setFlipY(m_startFlipY);
        setRotationX(m_startRotationX);
        setRotationY(m_startRotationY);

        GameObject::updateStartValues();
    }
};

class $modify(PlayerObject) {
    $override
    bool levelFlipping() {
        // ⏺️ fix particles not disappearing on mirror effect

        if (!LevelEditorLayer::get()) {
            return PlayerObject::levelFlipping();
        }

        return LevelEditorLayer::get()->isFlipping();
    }
};

bool ie::preUpdateMirrorEffect(LevelEditorLayer* lel) {
    float flip = lel->m_gameState.m_levelFlipping;
    bool flipping = flip != 0.f && flip != 1.f;

    // m_resetActiveObjects is only set to true after the flip animation ends

    if (!lel->m_resetActiveObjects) return flipping;
    lel->m_resetActiveObjects = false;

    for (const auto& object : CCArrayExt<GameObject>(lel->m_objects)) {
        object->setFlipX(object->m_startFlipX);
        object->setFlipY(object->m_startFlipY);
        object->setRotation(object->m_startRotationX);
    }

    return flipping;
}

void ie::updateMirrorEffect(LevelEditorLayer* lel, GameObject* object, bool flipping) {
    if (!flipping) return;

    static_cast<SMELevelEditorLayer*>(lel)->screenFlipObject(object);
}
