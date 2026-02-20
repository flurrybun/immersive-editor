#include "UpdateVisibility.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

bool shouldFadeObject(GameObject* object, bool isRight, short objectEnterType, int enterType) {
    if (object->m_ignoreFade) return false;
    if (object->m_isInvisibleBlock) return false;
    if (enterType == -14 || enterType == -15) return false;

    // some blocks like the solid black blocks in clutterfunk don't fade
    // this is the logic to handle that

    return (
        !object->m_intrinsicDontFade ||
        (object->m_isSolidColorBlock && object->m_baseOrDetailBlending) ||
        objectEnterType > -1 ||
        enterType != -2
    );
}

void applyFadeEffect(LevelEditorLayer* lel, GameObject* object, bool isRight) {
    CCPoint fadePos = object->getPosition();

    if (isRight) fadePos.x -= object->m_fadeMargin;
    else fadePos.x += object->m_fadeMargin;

    float distanceToEdge = isRight
        ? lel->m_gameState.m_cameraPosition2.x + lel->m_cameraWidth - fadePos.x
        : fadePos.x - lel->m_gameState.m_cameraPosition2.x;

    auto opacity = std::clamp(distanceToEdge / 70.f, 0.f, 1.f);
    object->setOpacity(opacity * 255.f);
}

void applyEnterEffect(GameObject* object, bool isRight, int enterType) {
    auto fakePL = reinterpret_cast<PlayLayer*>(GJBaseGameLayer::get());

    CCPoint prevEEP = fakePL->m_enterEffectPosition;
    fakePL->m_enterEffectPosition = object->getPosition();

    if (enterType == -15) {
        fakePL->applyCustomEnterEffect(object, isRight);
    } else {
        fakePL->applyEnterEffect(object, enterType, isRight);
    }

    fakePL->m_enterEffectPosition = prevEEP;
}

float ie::preUpdateFadeAndEnter(LevelEditorLayer* lel) {
    // global var only ever used in PlayLayer::applyCustomEnterEffect
    // why this is a global is entirely beyond me

#if GEODE_COMP_GD_VERSION == 22081
    // float* cameraRight = reinterpret_cast<float*>(geode::base::get() +
    //     GEODE_WINDOWS(0x6a304c)
    //     GEODE_INTEL_MAC(0x000000)
    //     GEODE_ARM_MAC(0x000000)
    //     GEODE_ANDROID64(0x000000)
    //     GEODE_ANDROID32(0x000000)
    //     GEODE_IOS(0x000000)
    // );
    // *cameraRight = lel->m_gameState.m_cameraPosition2.x + lel->m_cameraWidth;
#else
    #error "Incorrect GD version: no address for cameraRight global"
#endif

    return lel->m_gameState.m_cameraPosition2.x + lel->m_cameraWidth / 2;
}

void ie::updateFadeAndEnter(LevelEditorLayer* lel, GameObject* object, float cameraXCenter) {
    if (lel->m_playbackMode != PlaybackMode::Playing) return;
    if (object->m_isUIObject || object->m_isTrigger) return;

    bool isRight = object->getPositionX() > cameraXCenter;
    bool isEnter = isRight || object->m_enterType != -1;

    short objectEnterType = isEnter
        ? object->m_enterType
        : object->m_exitType;

    int enterType = isEnter
        ? lel->m_gameState.m_enterChannelMap[object->m_enterChannel]
        : lel->m_gameState.m_exitChannelMap[object->m_enterChannel];

    if (shouldFadeObject(object, isRight, objectEnterType, enterType)) {
        applyFadeEffect(lel, object, isRight);
    }

    if (!object->m_isDontEnter) {
        applyEnterEffect(object, isRight, enterType);
    }
}
