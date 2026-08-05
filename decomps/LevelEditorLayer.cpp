#include <Geode/Geode.hpp>
using namespace geode::prelude;

class _LevelEditorLayer : public LevelEditorLayer {
public:
    bool _activateTriggerEffect(EffectGameObject* object, float currentTime, float playTime, float boundTime, bool active);
    void _toggleGroupPreview(int id, bool enable);
    void _spawnGroupPreview(int groupID, float xPos, float delay, float currentTime, float playTime, float boundTime, bool ordered, bool active);

    void _updateBlendValues();
    void _updateObjectColors(CCArray* objects);
};

bool LevelEditorLayer::activateTriggerEffect(
    EffectGameObject* object,
    float currentTime,
    float playTime,
    float boundTime,
    bool active
) {
    if (
        object->m_isGroupDisabled ||
        (object->m_isTouchTriggered && !object->hasBeenActivated())
    ) return true;

    object->m_spawnTriggerDelay = object->m_spawnTriggerDelay;
    float interval = currentTime - object->m_spawnTriggerDelay - boundTime;

    if (object->m_isColorTrigger) {
        if (!m_previewMode) return true;

        bool instant =
            playTime - (object->m_spawnTriggerDelay + object->m_duration) <= 5.0 &&
            boundTime - (currentTime - object->m_spawnTriggerDelay) > 0.2;

        ColorAction* color = runColorEffect(
            object,
            object->m_targetColor,
            object->m_spawnTriggerDelay,
            interval,
            instant
        );

        if (color && object->m_objectID == 29 && object->m_tintGround) {
            runColorEffect(object, 1001, object->m_spawnTriggerDelay, interval, false);
        }

        // this is the only area in the entire function where it can return false
        // probably a leftover from when color triggers were the only kind of trigger

        return color != nullptr;
    }

    switch (object->m_objectID) {
        case 1006: { // pulse
            if (!m_previewMode) return true;

            float delay = object->m_spawnTriggerDelay + object->m_fadeInDuration + object->m_holdDuration + object->m_fadeOutDuration;
            if (delay >= playTime ) return true;

            PulseEffectAction* pulse = m_effectManager->runPulseEffect(
                object->m_targetGroupID,
                object->m_pulseTargetType == 1,
                object->m_fadeInDuration,
                object->m_holdDuration,
                object->m_fadeOutDuration,
                static_cast<PulseEffectType>(2 - (object->m_pulseMode != 0)),
                object->m_triggerTargetColor,
                object->m_hsvValue,
                object->m_copyColorID,
                object->m_pulseMainOnly,
                object->m_pulseDetailOnly,
                object->m_pulseExclusive,
                object->m_legacyHSV,
                object->m_uniqueID,
                object->m_controlID
            );

            pulse->m_startTime = interval;

            return true;
        }

        case 1007: { // alpha
            if (!m_previewMode) return true;

            OpacityEffectAction* action = m_effectManager->getOpacityActionForGroup(object->m_targetGroupID);

            if (
                action &&
                action->m_durationRelated != 0.f &&
                object->m_spawnTriggerDelay - action->m_durationRelated != 0.f
            ) {
                action->m_deltaTimeRelated = 0.0;
                action->m_deltaTime = 0.0;

                action->step(object->m_spawnTriggerDelay - action->m_durationRelated);
                m_effectManager->updateOpacityAction(action);
            }

            OpacityEffectAction* action2 = m_effectManager->runOpacityActionOnGroup(
                object->m_targetGroupID,
                object->m_duration,
                object->m_opacity,
                object->m_uniqueID,
                object->m_controlID
            );

            action2->m_durationRelated = object->m_spawnTriggerDelay;
            action2->m_deltaTimeRelated = interval;

            return true;
        }

        case 1049: // toggle
            toggleGroupPreview(object->m_targetGroupID, object->m_activateGroup);

        case 1268: { // spawn
            auto trigger = static_cast<SpawnTriggerGameObject*>(object);
            if (object->m_previewDisable) return true;

            if (trigger->m_spawnDelay > 0.f) {
                float delay = trigger->m_spawnDelay + object->m_spawnTriggerDelay;
                if (playTime >= delay) addDelayedSpawn(object, delay);
            } else {
                spawnGroupPreview(
                    object->m_targetGroupID,
                    object->spawnXPosition(),
                    object->m_spawnTriggerDelay,
                    currentTime,
                    playTime,
                    boundTime,
                    object->m_spawnOrdered,
                    active
                );
            }

            return true;
        }

        case 1616: { // stop
            auto trigger = static_cast<TriggerControlGameObject*>(object);

            if (trigger->m_customTriggerValue == GJActionCommand::Stop) {
                stopTriggersInGroup(object->m_targetGroupID, object->m_spawnTriggerDelay);
            }

            return true;
        }

        case 1934: // song
            if (!active) break;

            activateSongTrigger(static_cast<SongTriggerGameObject*>(object));
            return true;

        case 2900: { // gameplay rotation
            auto trigger = static_cast<RotateGameplayGameObject*>(object);

            if (trigger->m_changeChannel) {
                m_gameState.m_currentChannel = trigger->m_targetChannelID;
            }

            return true;
        }

        case 2903: // gradient
            triggerGradientCommand(static_cast<GradientTriggerObject*>(object));
            return true;

        case 2904: case 2905: case 2907: case 2909: case 2910: case 2911:
        case 2912: case 2913: case 2914: case 2915: case 2916: case 2917:
        case 2919: case 2920: case 2921: case 2922: case 2923: case 2924: { // shaders
            auto trigger = static_cast<ShaderGameObject*>(object);
            if (!m_previewShaders || trigger->m_editorDisabled) return true;

            if (m_shaderLayer && m_shaderLayer->m_timesyncShaderActions) {
                m_shaderLayer->m_state.m_time = object->m_spawnTriggerDelay + m_shaderLayer->m_state.m_startTime;
            }
            triggerShaderCommand(trigger);

            return true;
        }

        case 2999: // edit middleground
            updateMGOffsetY(object->m_moveOffset.y, 0.0, 0, 0.0, -1, -1);
            break;

        case 3602: // sfx
            if (!active) break;

            activateSFXTrigger(static_cast<SFXTriggerGameObject*>(object));
            return true;

        case 3603: // edit sfx
            if (!active) break;

            activateSFXEditTrigger(static_cast<SFXTriggerGameObject*>(object));
            return true;

        case 3605: // edit song
            if (!active) break;

            activateSongEditTrigger(static_cast<SongTriggerGameObject*>(object));
            return true;

        case 3606: // edit background speed
            updateBGArtSpeed(object->m_moveModX, object->m_moveModY);
            return true;

        case 3612: // edit middleground speed
            updateMGArtSpeed(object->m_moveModX, object->m_moveModY);
            return true;

        default:
            return true;
    }

    if (m_playbackActive && m_activateAudioTriggers) {
        activatedAudioTrigger(static_cast<SFXTriggerGameObject*>(object));
    }

    return true;
}

void _LevelEditorLayer::_toggleGroupPreview(int id, bool enable) {
    if (m_effectManager->isGroupEnabled(id) == enable) return;

    m_toggledGroupStates[id] = 2;
    CCArray* group = m_triggerGroups[id];

    if (!group) {
        group = CCArray::create();
        m_triggerGroups[id] = group;
        m_triggerGroupsDict->setObject(group, id);
    }

    for (auto object : CCArrayExt<GameObject*>(group)) {
        if (!object) break;

        if (enable) object->groupWasEnabled();
        else object->groupWasDisabled();
    }

    m_effectManager->toggleGroup(id, enable);
}

void _LevelEditorLayer::_spawnGroupPreview(
    int groupID,
    float xPos,
    float delay,
    float currentTime,
    float playTime,
    float boundTime,
    bool ordered,
    bool active
) {
    if (
        groupID <= 0 ||
        delay > playTime ||
        !m_effectManager->isGroupEnabled(groupID) ||
        m_spawnGroupDelays[groupID] >= delay
    ) return;

    m_spawnGroupDelays[groupID] = delay;
    CCArray* objects = getTriggerGroup(groupID);

    auto objectsExt = objects->asExt<EffectGameObject>();

    if (!m_sortedGroups[groupID]) {
        std::sort(objectsExt.begin(), objectsExt.end(), [](GameObject* a, GameObject* b) {
            return a->getPosition().x - b->getPosition().x;
        });

        m_sortedGroups[groupID] = true;
    }

    if (!ordered) {
        int spawnOrder = 0;

        for (auto object : objectsExt) {
            if (
                !object->m_isTrigger ||
                !object->m_isSpawnTriggered ||
                (!object->m_isMultiTriggered && object->m_activatedByPlayer1)
            ) return;

            object->m_spawnTriggerDelay = delay;
            object->m_spawnOrder = spawnOrder++;

            activateTriggerEffect(
                object, currentTime, playTime, boundTime, active && delay > m_drawGridLayer->m_oldPlaybackTime
            );
            object->triggerActivated(xPos);
        }

        return;
    }

    bool isInitial = true;
    float initialX;

    for (auto object : objectsExt) {
        if (
            !object->m_isTrigger ||
            !object->m_isSpawnTriggered ||
            (!object->m_isMultiTriggered && object->m_activatedByPlayer1)
        ) continue;

        float objectX = object->getPositionX();

        if (isInitial) {
            isInitial = false;
            initialX = objectX;
        } else {
            float orderDelay = (objectX - initialX) / 311.58011f;

            if (orderDelay != 0.f) {
                float orderDelayDelay = orderDelay + delay;
                if (playTime < orderDelayDelay) continue;

                DelayedSpawnNode* node = getDelayedSpawnNode();
                node->m_gameObject = object;
                node->m_spawnDelay = orderDelay;

                // why 9 of all numbers

                if (m_delayedSpawnNodes.size() >= 9) {
                    m_sortSpawnNodes = true;
                }

                continue;
            }
        }

        object->m_spawnTriggerDelay = delay;
        object->m_spawnOrder = 0;

        float active2 = active && delay > m_drawGridLayer->m_oldPlaybackTime;
        activateTriggerEffect(object, currentTime, playTime, boundTime, active2);
        object->triggerActivated(xPos);
    }
}

void _LevelEditorLayer::_updateObjectColors(CCArray* objects) {
    ccColor3B animatedChildColor = GameToolbox::transformColor(
        m_background->getColor(), 0.0, -0.30000001f, 0.40000001f
    );

    for (auto object : CCArrayExt<GameObject>(objects)) {
        int mode = object->getMainColorMode();
        float opacity = m_effectManager->activeOpacityForIndex(mode);
        object->getMainColor()->m_opacity = opacity;

        if (object->hasSecondaryColor()) {
            int mode2 = object->getSecondaryColorMode();
            float opacity2 = m_effectManager->activeOpacityForIndex(mode2);

            object->getSecondaryColor()->m_opacity = opacity2;
        }

        ccColor3B mainColor = m_effectManager->activeColorForIndex(mode);
        float mainOpacity = mode > 0 ? opacity : 1.f;
        ccHSVValue hsv = object->m_baseColor->m_hsv;

        for (int i = 0; i < object->m_groupCount; i++) {
            mainColor = m_effectManager->colorForGroupID(
                object->getGroupID(i), mainColor, !object->m_customSpriteColor
            );
        }

        if (
            // todo: make sure this is correct
            hsv.h != 0.f || hsv.s != 1.f || hsv.v != 1.f || hsv.absoluteSaturation != false
        ) {
            mainColor = m_effectManager->colorForEffect(mainColor, hsv);
        }

        object->updateMainColor(mainColor);

        for (int i = 0; i < object->m_groupCount; i++) {
            mainOpacity *= m_effectManager->opacityModForGroup(object->getGroupID(i));
        }

        object->m_baseColor->m_opacity = mainOpacity;

        if (object->hasSecondaryColor()) {
            int mode2 = object->getSecondaryColorMode();
            float opacity2 = m_effectManager->activeOpacityForIndex(mode2);
            ccHSVValue hsv2 = object->m_detailColor->m_hsv;

            ccColor3B secondaryColor = ccWHITE;
            float secondaryOpacity = mode2 > 0 ? opacity2 : 1.f;

            if (mode2 == 1012) {
                // todo: ?
            } else {
                secondaryColor = m_effectManager->activeColorForIndex(mode2);

                for (int i = 0; i < object->m_groupCount; i++) {
                    secondaryColor = m_effectManager->colorForGroupID(
                        object->getGroupID(i), secondaryColor, 0
                    );
                }
            }

            if (
                // todo: make sure this is correct
                hsv2.h != 0.f || hsv2.s != 1.f || hsv2.v != 1.f || hsv2.absoluteSaturation != false
            ) {
                secondaryColor = m_effectManager->colorForEffect(secondaryColor, hsv2);
            }

            object->updateSecondaryColor(secondaryColor);

            for (int i = 0; i < object->m_groupCount; i++) {
                secondaryOpacity *= m_effectManager->opacityModForGroup(object->getGroupID(i));
            }

            object->m_detailColor->m_opacity = secondaryOpacity;
        }

        static_cast<AnimatedGameObject*>(object)->updateChildSpriteColor(animatedChildColor);
    }
}

void _LevelEditorLayer::_updateBlendValues() {
    for (int i = 0; i != 1102; i++) {
        bool blend = m_effectManager->shouldBlend(i);

        m_blendingColors2[i] = blend != m_blendingColors[i];
        m_blendingColors[i] = blend;
    }

    m_blendingColors[1005] = true;
    m_blendingColors[1006] = true;
    m_blendingColors[1007] = true;
    m_blendingColors[0] = false;

    m_blendingColors2[1005] = false;
    m_blendingColors2[1006] = false;
    m_blendingColors2[1007] = false;
    m_blendingColors2[0] = false;
}
