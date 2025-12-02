#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include "UpdateVisibility.hpp"
#include "misc/ObjectEvent.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

// pulse rod balls are a special object (id 37) with the rod's properties duplicated over, but this method
// wouldn't work in the editor. instead, each rod gets a ball sprite that follows its pos/rot/scale/color/opacity

bool isPulseRod(GameObject* object) {
    return (
        object->m_objectID == 15 ||
        object->m_objectID == 16 ||
        object->m_objectID == 17
    );
}

class $modify(SPRBGameObject, GameObject) {
    struct Fields {
        CCSprite* ballSprite;
    };

    $override
    void setObjectColor(const ccColor3B& color) {
        if (m_fields->ballSprite) {
            m_fields->ballSprite->setColor(color);
        } else {
            GameObject::setObjectColor(color);
        }
    }
};

class $modify(SPRBLevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        ObjectEventListener objectListener;
        std::unordered_map<WeakRef<GameObject>, Ref<CCSprite>> pulseRods;
        short pulseRodIndex = 0;
    };

    $override
    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        generateRodIndex();

        m_fields->objectListener.bind([&](ObjectEvent* event) {
            if (!isPulseRod(event->object)) return ListenerResult::Propagate;

            if (event->isAdded) {
                addPulseRodBall(event->object);
            } else {
                removePulseRodBall(event->object);
            }

            return ListenerResult::Propagate;
        });

        return true;
    }

    $override
    void onPlaytest() {
        LevelEditorLayer::onPlaytest();

        generateRodIndex();
        std::string frame = fmt::format("rod_ball_{:02}_001.png", m_fields->pulseRodIndex);

        for (const auto& [rod, ball] : m_fields->pulseRods) {
            ball->setDisplayFrame(CCSpriteFrameCache::get()->spriteFrameByName(frame.c_str()));
        }
    }

    void addPulseRodBall(GameObject* object) {
        std::string frame = fmt::format("rod_ball_{:02}_001.png", m_fields->pulseRodIndex);
        CCSprite* ball = CCSprite::createWithSpriteFrameName("rod_ball_01_001.png");

        m_fields->pulseRods[object] = ball;
        static_cast<SPRBGameObject*>(object)->m_fields->ballSprite = ball;
    }

    void removePulseRodBall(GameObject* object) {
        auto it = m_fields->pulseRods.find(object);
        if (it == m_fields->pulseRods.end()) return;

        it->second->removeFromParent();
        m_fields->pulseRods.erase(it);

        static_cast<SPRBGameObject*>(object)->m_fields->ballSprite = nullptr;
    }

    void updatePulseRods() {
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
            bool isBallBlending = m_blendingColors[rodColorID];

            ball->setOpacity(rod->getOpacity());

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

    void generateRodIndex() {
        m_fields->pulseRodIndex = rand() % 3 + 1;
    }
};

void ie::updatePulseRodBalls(LevelEditorLayer* lel, float audioScale) {
    auto& pulseRods = static_cast<SPRBLevelEditorLayer*>(lel)->m_fields->pulseRods;

    if (FMODAudioEngine::get()->m_musicVolume <= 0.f) audioScale = 0.5f;
    else if (audioScale == -1.f) audioScale = 1.f;

    for (auto it = pulseRods.begin(); it != pulseRods.end(); ) {
        GameObject* rod = it->first.lock();
        CCSprite* ball = it->second;

        if (!rod) {
            ball->removeFromParent();
            it = pulseRods.erase(it);
            continue;
        }

        ++it;

        int rodColorID = rod->m_baseColor->m_colorID;
        bool isBallBlending = lel->m_blendingColors[rodColorID];

        ball->setOpacity(rod->getOpacity());

        CCNode* rodParent = rod->getParent();
        CCNode* ballParent = ball->getParent();
        CCSpriteBatchNode* ballLayer = isBallBlending ? lel->m_gameBlendingLayerB1 : lel->m_gameLayerB1;

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
        ball->setScale(audioScale);
    }
}
