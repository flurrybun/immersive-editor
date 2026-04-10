#include "UpdateVisibility.hpp"
#include "misc/Utils.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

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

void updateInvisibleBlock(LevelEditorLayer* lel, GameObject* object, const ie::FadeContext& context) {
    if (!object->m_isInvisibleBlock || !lel->m_previewMode) return;

    std::string style = Mod::get()->getSettingValue<std::string>("invisible-block-style");
    bool isInGame = lel->m_playbackMode == PlaybackMode::Playing || style == "In-Game";

    float layerOpacity = ie::isObjectLayerVisible(object, lel) ? 1.f : 0.2f;

    if (object->m_isSelected) {
        object->setOpacity(layerOpacity * 255.f);
        return;
    }

    const ccColor3B& lbgColor = context.lbgColor;
    float rightFadeBound = context.rightFadeBound;
    float leftFadeBound = context.leftFadeBound;
    float leftFadeWidth = context.leftFadeWidth;
    float rightFadeWidth = context.rightFadeWidth;

    if (style == "No Fade") {
        object->setOpacity(layerOpacity * 255.f);
        object->setGlowColor(GJEffectManager::getMixedColor(lel->m_lightBGColor, lbgColor, 0.9f));
        return;
    }

    if (lel->m_playbackMode != PlaybackMode::Playing) {
        float zoom = lel->m_objectLayer->getScale();

        rightFadeBound /= zoom;
        leftFadeBound /= zoom;
        leftFadeWidth /= zoom * 2.f;
        rightFadeWidth /= zoom * 2.f;
    }

    // decomp of PlayLayer::updateInvisibleBlock but with some extra stuff

    float objX = object->getRealPosition().x;

    if (objX <= lel->m_cameraUnzoomedX) objX += object->m_fadeMargin;
    else objX -= object->m_fadeMargin;

    // compute fade near edges of screen:

    // normally m_gameState.m_cameraPosition2.x
    float cameraX = -lel->m_objectLayer->getPositionX() / lel->m_objectLayer->getScale();
    float cameraCenterX = lel->m_halfCameraWidth + cameraX;

    float fadeFactor;

    if (objX <= cameraCenterX) {
        fadeFactor = 0.014285714f * (lel->m_halfCameraWidth - (cameraCenterX - objX));
    } else {
        fadeFactor = 0.02f * (lel->m_halfCameraWidth - (objX - cameraCenterX));
    }

    float cameraFade = std::clamp(fadeFactor, 0.f, 1.f) * 255.f;

    // compute fade near center of screen:

    float distance;
    float divisor;

    if (objX <= cameraX + rightFadeBound) {
        distance = (cameraX + leftFadeBound) - objX;
        divisor = leftFadeWidth;
    } else {
        distance = objX - cameraX - rightFadeBound;
        divisor = rightFadeWidth;
    }

    if (divisor <= 1.f) divisor = 1.f;

    // normally fixed at 0.05
    float minOpacity = isInGame ? 0.05f : 0.3f;

    float ratio = std::clamp(distance / divisor, 0.f, 1.f);
    float playerFade = (ratio * (1.f - minOpacity) + minOpacity) * 255.f;

    // set final opacity based on both fades:

    object->setOpacity(std::min(cameraFade, playerFade) * layerOpacity);

    // set glow opacity and color:

    if (object->m_glowSprite) {
        // normally fixed at 0.15
        float minOpacity = isInGame ? 0.15f : 0.25f;

        float glowFade = (ratio * (1.f - minOpacity) + minOpacity) * 255.f;
        glowFade = std::min(cameraFade, glowFade);

        GLubyte opacity = glowFade * object->m_opacityMod * layerOpacity;

        object->m_glowSprite->setOpacity(opacity);
        object->m_glowSprite->setChildOpacity(opacity);
    }

    float opacity = object->getOpacity() / 255.f;

    if (opacity > 0.8f) {
        float ratio = (1.0f - (opacity - 0.8f) / 0.2f) * 0.3f + 0.7f;
        object->setGlowColor(GJEffectManager::getMixedColor(lel->m_lightBGColor, lbgColor, ratio));
    } else {
        object->setGlowColor(lel->m_lightBGColor);
    }
}

ie::FadeContext ie::preUpdateFadeAndEnter(LevelEditorLayer* lel) {
    ccColor3B bgColor = lel->m_effectManager->activeColorForIndex(1000);
    int bgColorSum = bgColor.r + bgColor.g + bgColor.b;

    ccColor3B lbgColor = bgColorSum >= 150 ? ccWHITE : lel->m_effectManager->activeColorForIndex(1007);

    float screenRight = CCDirector::get()->getScreenRight();
    float playerX = screenRight * 0.5 - 75;

    ie::FadeContext context = {
        lel->m_gameState.m_cameraPosition2.x + lel->m_cameraWidth / 2,
        lbgColor,
        playerX + 110,
        playerX,
        screenRight - (playerX + 110) - 90,
        playerX - 30
    };

    // global var only ever used in PlayLayer::applyCustomEnterEffect
    // why this is a global is entirely beyond me

    // todo: detect amazon whenever it's updated (currently it's still on 2.2074)

    if (ie::isAmazon()) return context;

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

    return context;
}

void ie::updateFadeAndEnter(LevelEditorLayer* lel, GameObject* object, const ie::FadeContext& context) {
    if (object->m_isInvisibleBlock) {
        updateInvisibleBlock(lel, object, context);
    }

    if (
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
