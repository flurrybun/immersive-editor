#include <Geode/modify/LevelEditorLayer.hpp>
#include "misc/PlaytestEvent.hpp"
#include "UpdateVisibility.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(SFAELevelEditorLayer, LevelEditorLayer) {
    // uncomment this whenever i work out the issue with enter effects

    // struct Fields {
    //     PlaytestEventListener playtestListener;
    // };

    // $override
    // bool init(GJGameLevel* level, bool noUI) {
    //     if (!LevelEditorLayer::init(level, noUI)) return false;

    //     m_fields->playtestListener.bind([this](PlaytestEvent* event) {
    //         if (event->isPlaying()) return ListenerResult::Propagate;

    //         for (const auto& object : CCArrayExt<GameObject*>(m_objects)) {
    //             // resetting object scale is handled in AudioScale.cpp
    //             object->setRRotation(0.f);
    //         }

    //         return ListenerResult::Propagate;
    //     });

    //     return true;
    // }

    bool shouldFadeObject(GameObject* object, bool isRight, short customEnterType, int enterType) {
        if (object->m_ignoreFade) return false;
        if (object->m_isInvisibleBlock) return false;
        if (enterType == -14 || enterType == -15) return false;

        // some blocks like the solid black blocks in clutterfunk don't fade
        // this is the logic to handle that

        return (
            !object->m_intrinsicDontFade ||
            (object->m_isSolidColorBlock && object->m_baseOrDetailBlending) ||
            customEnterType > -1 ||
            enterType != -2
        );
    }

    void applyFadeEffect(GameObject* object, bool isRight) {
        CCPoint fadePos = object->getPosition();

        if (isRight) fadePos.x -= object->m_fadeMargin;
        else fadePos.x += object->m_fadeMargin;

        float distanceToEdge = isRight
            ? m_gameState.m_cameraPosition2.x + m_cameraWidth - fadePos.x
            : fadePos.x - m_gameState.m_cameraPosition2.x;

        auto opacity = std::clamp(distanceToEdge / 70.f, 0.f, 1.f);
        object->setOpacity(opacity * 255.f);
    }

    void applyEnterEffect(GameObject* object, bool isRight, int enterType) {
        auto fakePL = reinterpret_cast<PlayLayer*>(GJBaseGameLayer::get());

        CCPoint prevEEP = fakePL->m_enterEffectPosition;
        fakePL->m_enterEffectPosition = object->getPosition();

        if (enterType == -15) {
            // not fully working yet. for some reason custom enter effects
            // only seem to work on the left half of the screen?

            // fakePL->applyCustomEnterEffect(object, isRight);
        } else {
            fakePL->applyEnterEffect(object, enterType, isRight);
        }

        fakePL->m_enterEffectPosition = prevEEP;
    }
};

float ie::preUpdateFadeAndEnter(LevelEditorLayer* lel) {
    return lel->m_gameState.m_cameraPosition2.x + lel->m_cameraWidth / 2;
}

void ie::updateFadeAndEnter(LevelEditorLayer* lel, GameObject* object, float cameraXCenter) {
    if (lel->m_playbackMode != PlaybackMode::Playing) return;
    if (object->m_isUIObject || object->m_isTrigger) return;

    bool isRight = object->getPositionX() > cameraXCenter;
    bool isEnter = isRight || object->m_enterType != -1;

    short customEnterType = isEnter
        ? object->m_enterType
        : object->m_exitType;

    int enterType = isEnter
        ? lel->m_gameState.m_enterChannelMap[object->m_enterChannel]
        : lel->m_gameState.m_exitChannelMap[object->m_enterChannel];

    auto modLel = static_cast<SFAELevelEditorLayer*>(lel);

    if (modLel->shouldFadeObject(object, isRight, customEnterType, enterType)) {
        modLel->applyFadeEffect(object, isRight);
    }

    if (!object->m_isDontEnter) {
        modLel->applyEnterEffect(object, isRight, enterType);
    }
}
