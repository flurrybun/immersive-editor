#include <Geode/Geode.hpp>
using namespace geode::prelude;

class decomp_GJBaseGameLayer : public GJBaseGameLayer {
public:
    void decomp_loadUpToPosition(float position, int order, int channel);
    void decomp_processMoveActionsStep(float dt, bool visibleFrame);
    void decomp_checkSpawnObjects();
    bool decomp_shouldExitHackedLevel();
};

void decomp_GJBaseGameLayer::decomp_loadUpToPosition(float position, int order, int channel) {
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

void decomp_GJBaseGameLayer::decomp_processMoveActionsStep(float dt, bool visibleFrame) {
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

void decomp_GJBaseGameLayer::decomp_checkSpawnObjects() {
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

bool decomp_GJBaseGameLayer::decomp_shouldExitHackedLevel() {
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
