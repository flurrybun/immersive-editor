#include "core/SettingManager.hpp"
#include "core/UpdateVisibility.hpp"
#include "util/Utils.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

$bind_setting(g_showFadeAndEnter, "show-fade-and-enter");

void applyFadeEffect(LevelEditorLayer* lel, GameObject* object, float posX, bool isRight) {
    if (isRight) posX -= object->m_fadeMargin;
    else posX += object->m_fadeMargin;

    float distanceToEdge = isRight
        ? lel->m_gameState.m_cameraPosition2.x + lel->m_cameraWidth - posX
        : posX - lel->m_gameState.m_cameraPosition2.x;

    auto opacity = std::clamp(distanceToEdge / 70.f, 0.f, 1.f);
    object->setOpacity(opacity * 255.f);
}

void applyEnterEffect(GameObject* object, bool isRight, int enterType) {
    auto fakePL = reinterpret_cast<PlayLayer*>(GJBaseGameLayer::get());

    CCPoint prevEEP = fakePL->m_enterEffectPosition;
    fakePL->m_enterEffectPosition = object->getRealPosition();

    if (enterType == -15) {
        fakePL->applyCustomEnterEffect(object, isRight);
    } else if (!object->m_isDontEnter && !ie::isAmazon()) {
        fakePL->applyEnterEffect(object, enterType, isRight);
    }

    fakePL->m_enterEffectPosition = prevEEP;
}

float ie::preUpdateFadeAndEnter(LevelEditorLayer* lel) {
    if (!g_showFadeAndEnter) return 0;

    // global var only ever used in PlayLayer::applyCustomEnterEffect
    // why this is a global is entirely beyond me

    // todo: detect amazon whenever it's updated (currently it's still on 2.2074)

    if (ie::isAmazon()) return lel->m_gameState.m_cameraPosition2.x + lel->m_cameraWidth / 2;

#if GEODE_COMP_GD_VERSION == 22081
    float* cameraRight = reinterpret_cast<float*>(geode::base::get() +
        GEODE_WINDOWS(0x6c10bc)
        GEODE_INTEL_MAC(0x9a3ebc)
        GEODE_ARM_MAC(0x8bb30c)
        GEODE_ANDROID64(0x341ee4)
        GEODE_ANDROID32(0xac5b48)
        GEODE_IOS(0x885cf4)
    );
    *cameraRight = lel->m_gameState.m_cameraPosition2.x + lel->m_cameraWidth;
#else
    #error "Incorrect GD version: no address for cameraRight global"
#endif

    return lel->m_gameState.m_cameraPosition2.x + lel->m_cameraWidth / 2;
}

void ie::updateFadeAndEnter(LevelEditorLayer* lel, GameObject* object, float cameraXCenter) {
    if (
        !g_showFadeAndEnter ||
        lel->m_playbackMode != PlaybackMode::Playing ||
        object->m_isUIObject ||
        object->m_isTrigger
    ) return;

    // gd has two different values for fading/entering:
    // m_isDontFade/m_isDontEnter and m_isDontFade/m_ignoreEnter

    // the former corresponds with the checkboxes in the editor extras
    // whereas the latter is used in PlayLayer::updateVisibility to determine if objects should fade/enter
    // the only difference is the latter is set to true in ParticleGameObject::customSetup

    if (object->m_objectID == 2065) return;

    CCPoint objectPos = object->getRealPosition();

    bool isRight;
    gd::vector<int>* channelMap;
    int enterExitType;

    if (object->m_enterType != -1) {
        isRight = true;
        channelMap = &lel->m_gameState.m_enterChannelMap;
        enterExitType = object->m_enterType;
    } else if (object->m_exitType != -1) {
        isRight = false;
        channelMap = &lel->m_gameState.m_exitChannelMap;
        enterExitType = object->m_exitType;
    } else {
        isRight = (lel->m_halfCameraWidth + lel->m_gameState.m_cameraPosition2.x) < objectPos.x;
        channelMap = isRight ? &lel->m_gameState.m_enterChannelMap : &lel->m_gameState.m_exitChannelMap;
        enterExitType = object->m_exitType;
    }

    int enterExitChannel = (*channelMap)[object->m_enterChannel];

    if (object->m_isUIObject) {
        enterExitType = -14;
        enterExitChannel = -14;
    }

    if (!object->m_isInvisibleBlock) {
        if (object->m_isDontFade || enterExitChannel == -14) {
            object->setOpacity(255);
        } else {
            bool skipFade = object->m_intrinsicDontFade &&
                (!object->m_isSolidColorBlock || !object->m_baseOrDetailBlending) &&
                enterExitType < 0 &&
                enterExitChannel == -2;

            if (skipFade) {
                object->setOpacity(255);
            } else if (enterExitChannel != -15) {
                applyFadeEffect(lel, object, objectPos.x, isRight);
            }
        }
    }

    applyEnterEffect(object, isRight, enterExitChannel);
}
