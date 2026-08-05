#include "freestanding.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class _GJBaseGameLayer : public GJBaseGameLayer {
public:
    void _loadUpToPosition(float position, int order, int channel);
    void _processMoveActionsStep(float dt, bool visibleFrame);
    void _processDynamicObjectActions(int type, float dt);
    void _processAreaActions(float deltaTime, bool backwards);

    void _checkSpawnObjects();
    bool _shouldExitHackedLevel();
};

void _GJBaseGameLayer::_loadUpToPosition(float position, int order, int channel) {
    resetSPTriggered();
    resetSongTriggerValues();

    CCPoint positionPoint = { position, 0.f };
    float duration = timeForPos(positionPoint, order, channel, false, 0);
    if (duration > 3600.f) duration = 3600.f;

    int totalSteps = ceilf(duration / 0.016666668f) + 1;

    CCPoint savedPlayerPos = m_player1->getPosition();
    m_player1->setPosition({ 0.f, 0.f });

    CCPoint prevPlayerPos = { 0.f, 0.f };
    float elapsed = 0.f;

    for (int step = 0; step < totalSteps; step++) {
        float dt = (elapsed + 0.016666668f > duration)
            ? duration - elapsed
            : 0.016666668f;

        elapsed += dt;

        m_gameState.m_totalTime += dt;
        m_gameState.m_levelTime += dt;

        CCPoint currentPos = posForTime(elapsed);
        CCPoint playerPos = m_player1->getPosition();

        float deltaX = currentPos.x - playerPos.x;
        float deltaY = currentPos.y - playerPos.y;

        m_player1->setPosition(currentPos);
        prevPlayerPos = currentPos;

        m_effectManager->updateEffects(dt);
        m_effectManager->preCollisionCheck();

        m_spawnTuples.clear();

        m_effectManager->updateSpawnTriggers(dt);

        m_spawnTuples.clear();

        m_effectManager->m_unk780 = deltaX;
        m_effectManager->m_unk784 = deltaY;
        m_effectManager->m_unk788 = deltaX;
        m_effectManager->m_unk78C = deltaY;

        m_effectManager->updateTimers(dt, 1.f);

        m_effectManager->prepareMoveActions(dt, false);

        bool isLastStep = step == totalSteps - 1;
        processMoveActionsStep(dt, isLastStep);

        m_effectManager->postMoveActions();
        updateCollisionBlocks();
        checkSpawnObjects();

        m_gameState.m_currentChannel = m_gameState.m_rotateChannel;

        checkSpawnObjects();

        m_effectManager->postCollisionCheck();
        m_effectManager->processColors();
    }

    m_player1->m_maybeReverseSpeed = 0.f;
    m_player1->m_maybeReverseAcceleration = 0.f;
    m_player1->m_yVelocity = 0.f;

    m_player1->enablePlayerControls();
    m_player2->enablePlayerControls();

    m_effectManager->m_unk780 = 0.f;
    m_effectManager->m_unk784 = 0.f;
    m_effectManager->m_unk788 = 0.f;
    m_effectManager->m_unk78C = 0.f;

    m_player1->setPosition(savedPlayerPos);
}

void _GJBaseGameLayer::_processMoveActionsStep(float dt, bool visibleFrame) {
    m_unked0 = 0;
    m_disabledObjectsCount = 0;
    m_unked8 = 0;
    m_areaObjectsCount = 0;

    for (auto& action : m_gameState.m_dynamicMoveActions) {
        int groupID = action.m_gameObject1->m_targetGroupID;

        CCMoveCNode* node = m_effectManager->tryGetMoveCommandNode(groupID);
        if (!node) continue;

        node->m_unk0d1 = true;
    }

    processDynamicObjectActions(1, dt);
    processTransformActions(visibleFrame);
    processRotationActions();
    processDynamicObjectActions(0, dt);
    processMoveActions();
    processPlayerFollowActions(dt);
    processAdvancedFollowActions(dt);
    processFollowActions();
    processAreaActions(dt, visibleFrame);

    if (!m_isEditor || m_disabledObjectsCount <= 0) return;

    for (int i = 0; i < m_disabledObjectsCount; i++) {
        GameObject* object = m_disabledObjects[i];
        object->quickUpdatePosition();
    }
}

void _GJBaseGameLayer::_processDynamicObjectActions(int type, float dt) {
    if (type < 0 || type > 1) return;

    bool isMoveAction = type == 0;
    gd::vector<DynamicObjectAction>& actions = isMoveAction
        ? m_gameState.m_dynamicMoveActions
        : m_gameState.m_dynamicRotateActions;

    for (unsigned int i = 0; i < actions.size(); i++) {
        DynamicObjectAction& action = actions[i];
        if (action.m_unkBool3 && !action.m_unkBool4) continue;

        EffectGameObject* trigger = action.m_gameObject1;

        GroupCommandObject2* cmd = m_effectManager->getTempGroupCommand();
        cmd->m_targetGroupID = action.m_targetGroupID;
        cmd->m_centerGroupID = action.m_centerGroupID;
        cmd->m_triggerUniqueID = trigger->m_uniqueID;
        cmd->m_controlID = action.m_controlID;

        CCMoveCNode* moveNode = nullptr;

        if (isMoveAction) {
            moveNode = m_effectManager->getMoveCommandNode(cmd);
        } else {
            m_effectManager->m_unkMap5c8[action.m_targetGroupID].push_back(cmd);
            m_effectManager->registerRotationCommand(cmd, m_effectManager->m_unk798);
        }

        float elapsed = action.m_unkFloat1;
        float duration = trigger->m_duration;
        float timeRemaining = duration - elapsed;

        bool isFinished = (
            action.m_unkBool2 ||
            action.m_unkBool3 ||
            (timeRemaining <= 0.f && duration != -1.f && !action.m_unkBool1)
        );

        if (isFinished) {
            if (isMoveAction) {
                moveNode->m_unk078 = true;
            } else {
                cmd->m_finishRelated = true;
                cmd->stepTransformCommand(0.f, false, true);
            }

            if (action.m_unkBool3) {
                action.m_unkBool4 = false;
            } else {
                actions.erase(actions.begin() + i);
                i--;
            }

            return;
        }

        if (action.m_unkBool1) {
            action.m_unkBool1 = false;
        } else if (duration != -1.f) {
            action.m_unkFloat1 += std::min(dt, timeRemaining);
        }

        if (isMoveAction) {
            float easedT = GameToolbox::getEasedValue(
                action.m_unkFloat1 / duration, static_cast<int>(trigger->m_easingType), trigger->m_easingRate
            );

            if (easedT == 1.f && timeRemaining > dt) {
                easedT = (action.m_unkFloat3 >= 1.f) ? (1.f - 0.0001f) : (1.f + 0.0001f);
            }

            action.m_unkFloat3 = easedT;

            float prevAccumulated = action.m_unkFloat2;
            CCPoint targetPos = action.m_gameObject3->getRealPosition();
            CCPoint sourcePos = action.m_gameObject2->getRealPosition();
            CCPoint delta = targetPos - sourcePos;

            float newAccumulated = easedT * duration;
            action.m_unkFloat2 = newAccumulated;

            float distRemaining = duration - prevAccumulated;
            float ratio = distRemaining != 0.f ? (newAccumulated - prevAccumulated) / distRemaining : 0.f;

            if (duration <= 0.f) ratio = 1.f;

            if (trigger->m_isDirectionFollowSnap360) {
                // move trigger "direction mode" is enabled

                float timeRatio = duration > 0.f ? (distRemaining / duration) : 1.f;
                delta = ccpNormalize(delta) * (timeRatio * trigger->m_directionModeDistance);
            }

            float dx = ratio * delta.x;
            float dy = ratio * delta.y;

            moveNode->m_unk038 += dx;
            moveNode->m_unk040 += dy;
            moveNode->m_unk090 += dx;
            moveNode->m_unk098 += dy;
        } else {
            float angleDelta = getRotateCommandAngleDelta(
                action.m_gameObject1,
                action.m_gameObject2,
                action.m_gameObject4,
                action.m_gameObject3,
                action.m_gameObject5,
                action.m_gameObject6,
                action.m_gameObject7,
                action.m_gameObject8
            );

            cmd->m_currentRotateOrTransformDelta = angleDelta / std::max(trigger->m_dynamicModeEasing, 1);
            cmd->stepTransformCommand(0.f, false, true);
        }
    }
}

void _GJBaseGameLayer::_processAreaActions(float deltaTime, bool backwards) {
    m_gameState.m_unkUint3 = m_gameState.m_commandIndex - 1;

    processAreaEffects(&m_gameState.m_scaleEffectInstances, GJAreaActionType::Scale, deltaTime, backwards);
    processAreaEffects(&m_gameState.m_rotateEffectInstances, GJAreaActionType::Rotate, deltaTime, backwards);
    processAreaEffects(&m_gameState.m_moveEffectInstances, GJAreaActionType::Move, deltaTime, backwards);

    for (int i = 0; i < m_processedAreaObjectsCount; i++) {
        GameObject* obj = m_processedAreaObjects[i];

        if (obj->m_unk4C8 < m_gameState.m_commandIndex) {
            obj->m_isDirty = true;
            obj->m_isObjectRectDirty = true;
            obj->m_isOrientedBoxDirty = true;

            if (resetAreaObjectValues(obj, false)) {
                updateObjectSection(obj);
            }
        } else if (!backwards) {
            continue;
        }

        obj->setRRotation(0);
        obj->setRScale(1.f);
    }

    m_processedAreaObjectsCount = 0;

    for (int i = 0; i < m_areaObjectsCount; i++) {
        m_processedAreaObjectsCount++;
        GameObject* obj = m_areaObjects[i];

        if (m_processedAreaObjectsIndex > i) {
            m_processedAreaObjects[i] = obj;
        } else {
            m_processedAreaObjects.push_back(obj);
            ++m_processedAreaObjectsIndex;
        }
    }

    if (!m_areaObjectsUpdated) return;

    for (int i = 0; i < m_areaObjectsCount; i++) {
        GameObject* obj = m_areaObjects[i];

        obj->m_lastPosition.x = obj->m_positionX;
        obj->m_lastPosition.y = obj->m_positionY;
        obj->m_isObjectRectDirty = true;
        obj->m_isOrientedBoxDirty = true;
        obj->m_customScaleX = obj->m_scaleX;
        obj->m_customScaleY = obj->m_scaleY;
    }
}

void _GJBaseGameLayer::_checkSpawnObjects() {
    CCPoint playerPos = m_isPlatformer
        ? posForTime(m_gameState.m_levelTime)
        : m_player1->getPosition();

    int channel = m_gameState.m_currentChannel;

    while (true) {
        CCArray* objects = getActiveOrderSpawnObjects();

        // m_spawnChannelRelated0 = spawnObjectIndexForChannelMap
        // m_spawnChannelRelated1 = backwardsChannelMap

        int objectIndex = m_gameState.m_spawnChannelRelated0[channel];
        bool isReversed = m_gameState.m_spawnChannelRelated1[channel];

        if (objects->count() <= objectIndex) break;

        auto object = static_cast<EffectGameObject*>(objects->objectAtIndex(objectIndex));
        bool isTouchTriggered = false;

        if (!object->m_isTouchTriggered) {
            if (!m_isPlatformer) {
                if (!m_player1->m_isSideways) {
                    if (isReversed) {
                        if (playerPos.x < object->m_speedStart.x) break;
                    } else {
                        if (object->m_speedStart.x < playerPos.x) break;
                    }
                } else {
                    if (isReversed) {
                        if (playerPos.y < object->m_speedStart.y) break;
                    } else {
                        if (object->m_speedStart.y < playerPos.y) break;
                    }
                }
            } else {
                if (playerPos.x < object->m_speedStart.x) break;
            }
        } else {
            if (!object->m_activated) break;
            isTouchTriggered = true;
        }

        if (!object->m_isGroupDisabled && !isTouchTriggered) {
            if (object->m_objectID == 1268) {
                auto spawnObject = static_cast<SpawnTriggerGameObject*>(object);

                gd::vector<int> remapKeys = {};
                spawnObject->updateRemapKeys(remapKeys);
            }

            object->triggerObject(this, 0, nullptr);
        }

        m_gameState.m_spawnChannelRelated0[channel]++;
    }

    applyTimeWarp(m_gameState.m_timeWarp);
}

bool _GJBaseGameLayer::_shouldExitHackedLevel() {
    int max = 0;
    float numNonEmpty = 0;
    float numEmpty = 0;

    for (auto columnSizes : m_sectionSizes) {
        for (auto columnSize : *columnSizes) {
            max = std::max(max, columnSize);

            if (columnSize > 0) numNonEmpty++;
            else numEmpty++;
        }
    }

    int threshold = numEmpty / numNonEmpty > 0.9f ? 4000 : 8000;
    return max > threshold;
}
