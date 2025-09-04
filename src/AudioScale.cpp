#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(LevelEditorLayer) {
    struct Fields {
        int prevAudioTrack = -1;
        int prevSongID = -1;

        std::unordered_map<WeakRef<GameObject>, Ref<CCSprite>> pulseRods;
        short pulseRodIndex = 0;
    };

    $override
    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        FMODAudioEngine::get()->enableMetering();

        generateRodIndex();

        for (const auto& object : CCArrayExt<GameObject*>(m_objects)) {
            tryAddPulseRodBall(object);
        }

        return true;
    }

    $override
    GameObject* createObject(int p0, CCPoint p1, bool p2) {
        GameObject* object = LevelEditorLayer::createObject(p0, p1, p2);

        tryAddPulseRodBall(object);

        return object;
    }

    void generateRodIndex() {
        m_fields->pulseRodIndex = rand() % 3 + 1;
    }

    void tryAddPulseRodBall(GameObject* object) {
        if (
            object->m_objectID != 15 &&
            object->m_objectID != 16 &&
            object->m_objectID != 17
        ) return;

        std::string frame = fmt::format("rod_ball_{:02}_001.png", m_fields->pulseRodIndex);
        CCSprite* ball = CCSprite::createWithSpriteFrameName("rod_ball_01_001.png");

        m_fields->pulseRods[object] = ball;
    }

    $override
    void onPlaytest() {
        LevelEditorLayer::onPlaytest();

        // if (m_audioEffectsLayer) {
        //     m_audioEffectsLayer->m_timeElapsed = 0.f;
        // }

        generateRodIndex();
        std::string frame = fmt::format("rod_ball_{:02}_001.png", m_fields->pulseRodIndex);

        for (const auto& [rod, ball] : m_fields->pulseRods) {
            ball->setDisplayFrame(CCSpriteFrameCache::get()->spriteFrameByName(frame.c_str()));
        }
    }

    $override
    void levelSettingsUpdated() {
        LevelEditorLayer::levelSettingsUpdated();

        bool hasAudioChanged = m_fields->prevAudioTrack != m_level->m_audioTrack ||
            m_fields->prevSongID != m_level->m_songID;

        m_fields->prevAudioTrack = m_level->m_audioTrack;
        m_fields->prevSongID = m_level->m_songID;

        if (!hasAudioChanged) return;

        if (m_audioEffectsLayer) {
            m_audioEffectsLayer->removeFromParent();
            m_audioEffectsLayer = nullptr;
        }

        if (m_level->m_songID == 0) {
            m_audioEffectsLayer = AudioEffectsLayer::create(LevelTools::getAudioString(m_level->m_audioTrack));
            m_objectLayer->addChild(m_audioEffectsLayer);
        }
    }

    $override
    void updateVisibility(float dt) {
        LevelEditorLayer::updateVisibility(dt);

        updatePulseRods();

        if (m_playbackMode == PlaybackMode::Not && !m_editorUI->m_isPlayingMusic) {
            for (const auto& object : CCArrayExt<GameObject*>(m_objects)) {
                object->setRScale(1.f);
            }

            for (const auto& [rod, ball] : m_fields->pulseRods) {
                ball->setScale(1.f);
            }

            return;
        }

        if (m_audioEffectsLayer) {
            GameManager::get()->m_playLayer = reinterpret_cast<PlayLayer*>(GJBaseGameLayer::get());
            m_audioEffectsLayer->audioStep(dt);
            GameManager::get()->m_playLayer = nullptr;
        }

        float audioScale = m_audioEffectsLayer
            ? m_audioEffectsLayer->m_audioScale
            : FMODAudioEngine::get()->getMeteringValue();

        if (m_player1) m_player1->m_audioScale = audioScale;
        if (m_player2) m_player2->m_audioScale = audioScale;

        for (const auto& object : CCArrayExt<GameObject*>(m_objects)) {
            if (!object->m_usesAudioScale || object->m_hasNoAudioScale) continue;

            // orbs have their own audio scale logic in RingObject::setRScale, which only runs when
            // m_editorEnabled is false? otherwise it's the same as GameObject::setRScale. weird

            object->m_editorEnabled = false;

            if (object->m_customAudioScale) {
                float min = object->m_minAudioScale;
                float max = object->m_maxAudioScale;

                object->setRScale(min + audioScale * (max - min));
            } else {
                object->setRScale(audioScale);
            }

            object->m_editorEnabled = true;
        }

        for (const auto& [rod, ball] : m_fields->pulseRods) {
            ball->setScale(audioScale);
        }
    }

    void updatePulseRods() {
        // not sure how necessary using weak refs is here because deleted
        // objects don't seem to be freed until exiting the editor
        // i guess it can't hurt to be safe?

        for (auto it = m_fields->pulseRods.begin(); it != m_fields->pulseRods.end(); ) {
            GameObject* rod = it->first.lock();
            CCSprite* ball = it->second;

            if (!rod) {
                ball->removeFromParent();
                it = m_fields->pulseRods.erase(it);
                continue;
            }

            ++it;

            int rodColorID = rod->m_baseColor->m_colorID;
            bool isBallBlending = false;

            if (rodColorID == 0 || rodColorID == 1004) rodColorID = 1005;

            if (ColorActionSprite* colorAction = m_effectManager->m_colorActionSpriteVector[rodColorID]) {
                ball->setColor(colorAction->m_color);
                isBallBlending = m_blendingColors[rodColorID];
            }

            CCNode* rodParent = rod->getParent();
            CCNode* ballParent = ball->getParent();
            CCSpriteBatchNode* ballLayer = isBallBlending ? m_gameBlendingLayerB1 : m_gameLayerB1;

            if (rodParent && !ballParent) {
                ballLayer->addChild(ball);
            } else if (!rodParent && ballParent) {
                ball->removeFromParent();
                continue;
            } else if (ballParent && ballParent != ballLayer) {
                ball->removeFromParent();
                ballLayer->addChild(ball);
            } else if (!rodParent && !ballParent) {
                continue;
            }

            CCPoint relativeBallPos = ccp(rod->getContentWidth() * 0.5f, rod->getContentHeight() + 10.f);
            CCPoint ballPos = ballLayer->convertToNodeSpace(rod->convertToWorldSpace(relativeBallPos));

            ball->setPosition(ballPos);
        }
    }
};

class $modify(GJBaseGameLayer) {
    // i can't hook ~LevelEditorLayer() but this works just fine

    $override
    void destructor() {
        GJBaseGameLayer::~GJBaseGameLayer();
        FMODAudioEngine::get()->disableMetering();
    }
};
