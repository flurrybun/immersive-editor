#include <Geode/Geode.hpp>
using namespace geode::prelude;

float _cameraRight;

#define BITSET_GET(bits, idx) ((bits)[(idx) >> 5] & (1u << ((idx) & 31)))
#define BITSET_SET(bits, idx) ((bits)[(idx) >> 5] |= (1u << ((idx) & 31)))

class decomp_PlayLayer : public PlayLayer {
public:
    void decomp_updateVisibility(float dt);
};

// to be honest i haven't tested this for accuracy at all :P

void decomp_PlayLayer::decomp_updateVisibility(float dt) {
    _cameraRight = m_cameraWidth + m_gameState.m_cameraPosition2.x;

    preUpdateVisibility(dt);
    m_effectManager->processColors();

    // setup colors

    ccColor3B p1Color = m_effectManager->activeColorForIndex(1005);
    m_effectManager->calculateLightBGColor(p1Color);

    ccColor3B monsterGlowColor = GameToolbox::transformColor(
        m_background->getColor(), { 0.f, -0.3f, 0.4f, true, true }
    );

    ccColor3B bgColor = m_effectManager->activeColorForIndex(1000);
    ccColor3B lbgColor = m_effectManager->activeColorForIndex(1007);

    m_lightBGColor = GameToolbox::transformColor(bgColor, { 0.f, -0.2f, 0.2f, true, true });

    std::fill(m_keyPulses.m_bits.begin(), m_keyPulses.m_bits.end(), 0);

    float screenRight = CCDirector::sharedDirector()->getScreenRight();
    float leftFadeBound = screenRight * 0.5f - 75.f;

    int bgColorSum = (int)bgColor.r + (int)bgColor.g + (int)bgColor.b;
    ccColor3B lineColor = (bgColorSum >= 150) ? ccWHITE : lbgColor;

    bool player1Dead = m_player1->m_isDead;
    bool isFlipping = m_gameState.m_levelFlipping != 0.f && m_gameState.m_levelFlipping != 1.f;

    // audio scale

    float audioScale = m_skipAudioStep
        ? FMODAudioEngine::get()->getMeteringValue()
        : m_audioEffectsLayer->m_audioScale;

    if ((m_isPracticeMode && !m_practiceMusicSync) || m_isSilent) audioScale = 0.5f;

    m_player1->m_audioScale = audioScale;
    m_player2->m_audioScale = audioScale;

    size_t blendingColorsSize = m_blendingColors.size();
    auto& pulseBits = m_keyPulses.m_bits;

    for (int i = 0; i < m_activeObjectsCount; i++) {
        GameObject* object = m_activeObjects[i];
        int mainColorKeyIndex = object->m_mainColorKeyIndex;

        // to be honest i have no idea what this logic is doing

        if (mainColorKeyIndex < 1) {
            object->updateMainColor();
            object->updateSecondaryColor();
        } else {
            if (!BITSET_GET(pulseBits, mainColorKeyIndex)) {
                m_keyColors[mainColorKeyIndex] = object->colorForMode(object->m_activeMainColorID, true);
                m_keyOpacities[mainColorKeyIndex] = object->opacityModForMode(object->m_activeMainColorID, true);
                BITSET_SET(pulseBits, mainColorKeyIndex);
            }

            object->updateMainColor();
            object->m_baseColor->m_opacity = m_keyOpacities[mainColorKeyIndex];

            if (object->m_colorSprite) {
                int detailColorKeyIndex = object->m_detailColorKeyIndex;

                if (!BITSET_GET(pulseBits, detailColorKeyIndex)) {
                    m_keyColors[detailColorKeyIndex] = object->colorForMode(object->m_activeDetailColorID, false);
                    m_keyOpacities[detailColorKeyIndex] = object->opacityModForMode(object->m_activeDetailColorID, false);
                    BITSET_SET(pulseBits, detailColorKeyIndex);
                }

                object->updateSecondaryColor();
                object->m_detailColor->m_opacity = m_keyOpacities[detailColorKeyIndex];
            }
        }

        if (object->m_isActivated && blendingColorsSize != 0) {
            bool mainIsBlending = m_blendingColors.contains(object->m_activeMainColorID);
            bool detailIsBlending = object->m_colorSprite && m_blendingColors.contains(object->m_activeDetailColorID);

            if (mainIsBlending || detailIsBlending) {
                object->addMainSpriteToParent(false);
                object->addColorSpriteToParent(false);
            }
        }

        object->activateObject();

        // setup enter/exit effects

        m_enterEffectPosition = object->getRealPosition();

        bool isRight;
        gd::vector<int>* channelMap;
        int enterExitType;

        if (object->m_enterType != -1) {
            isRight = true;
            channelMap = &m_gameState.m_enterChannelMap;
            enterExitType = object->m_enterType;
        } else if (object->m_exitType != -1) {
            isRight = false;
            channelMap = &m_gameState.m_exitChannelMap;
            enterExitType = object->m_exitType;
        } else {
            isRight = (m_halfCameraWidth + m_gameState.m_cameraPosition2.x) < m_enterEffectPosition.x;
            channelMap = isRight ? &m_gameState.m_enterChannelMap : &m_gameState.m_exitChannelMap;
            enterExitType = object->m_exitType;
        }

        int enterExitChannel = (*channelMap)[object->m_enterChannel];

        if (object->m_isUIObject) {
            enterExitType = -14;
            enterExitChannel = -14;
        }

        // update random object stuff

        if (object->getHasSyncedAnimation())
            static_cast<EnhancedGameObject*>(object)->updateSyncedAnimation(m_gameState.m_totalTime, -1);

        if (object->getHasRotateAction())
            static_cast<EnhancedGameObject*>(object)->updateRotateAction(dt);

        if (object->m_hasAnimatedChild)
            static_cast<AnimatedGameObject*>(object)->updateChildSpriteColor(monsterGlowColor);

        if (object->getType() == GameObjectType::Collectible)
            static_cast<EffectGameObject*>(object)->updateInteractiveHover(m_hoverNode->getPosition().y);

        if (object->m_objectID == 143)
            object->setGlowColor(m_lightBGColor);

        if (object->m_unk3F8)
            continue;

        if (object->m_usesAudioScale && !object->m_hasNoAudioScale)
            object->setRScale(audioScale);

        if (object->m_customGlowColor)
            object->setGlowColor(object->m_glowColorIsLBG ? lbgColor : m_lightBGColor);

        if (object->m_isInvisibleBlock) {
            if (!player1Dead) {
                updateInvisibleBlock(
                    object,
                    leftFadeBound + 110.0f,
                    leftFadeBound,
                    (screenRight - (leftFadeBound + 110.0f)) - 90.0f,
                    leftFadeBound - 30.0f,
                    lineColor
                );
            } else {
                object->setGlowColor(m_lightBGColor);
                object->setOpacity(255);
                object->setGlowOpacity(255);
            }
        } else {
            if (object->m_ignoreFade || enterExitChannel == -14) {
                object->setOpacity(255);
            } else {
                bool skipFade = object->m_intrinsicDontFade
                    && (!object->m_isSolidColorBlock || !object->m_baseOrDetailBlending)
                    && enterExitType < 0
                    && enterExitChannel == -2;

                if (skipFade) {
                    object->setOpacity(255);
                } else if (enterExitChannel != -15) {
                    float fadeX = m_enterEffectPosition.x;
                    fadeX += isRight ? -object->m_fadeMargin : object->m_fadeMargin;

                    float distFromEdge = isRight
                        ? (m_gameState.m_cameraPosition2.x + m_cameraWidth) - fadeX
                        : fadeX - m_gameState.m_cameraPosition2.x;

                    object->setOpacity(std::clamp(distFromEdge / 70.f, 0.f, 1.f) * 255.f);
                }
            }
        }

        if (enterExitChannel != -15) applyEnterEffect(object, enterExitChannel, isRight);
        else if (!object->m_ignoreEnter) applyCustomEnterEffect(object, isRight);

        if (isFlipping) {
            screenFlipObject(object);
        } else if (m_resetActiveObjects) {
            object->setFlipX(object->m_startFlipX);
            object->setFlipY(object->m_startFlipY);
            object->setRotationX(object->m_startRotationX);
            object->setPosition(object->getPosition());
        }
    }

    updateEnterEffects(dt);
    processAreaVisualActions(dt);
    updateParticles(dt);

    m_resetActiveObjects = false;
    m_blendingColors.clear();

    if ((m_isDebugDrawEnabled && m_isPracticeMode) || (m_hitboxesOnDeath && m_playerDied)) {
        updateDebugDraw();
    }
}
