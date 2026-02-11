#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include "UpdateVisibility.hpp"
#include "misc/ObjectEvent.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

// pulse rod balls are a special object (id 37) with the rod's properties duplicated over, but this method
// wouldn't work in the editor. instead, each rod gets a ball sprite that follows its pos/rot/scale/color/opacity

bool isPulseRodID(int key) {
    return (
        key == 15 ||
        key == 16 ||
        key == 17
    );
}

bool isPulseRod(GameObject* object) {
    return isPulseRodID(object->m_objectID);
}

// creating a new class avoids hooking setObjectColor with a fields accessor (very costly)

class PulseRodGameObject : public GameObject {
public:
    Ref<CCSprite> m_ball;

    void setObjectColor(const ccColor3B& color) override {
        if (m_ball) {
            m_ball->setColor(color);
        } else {
            GameObject::setObjectColor(color);
        }
    }
};

class $modify(GameObject) {
    $override
    static GameObject* createWithKey(int key) {
        if (!isPulseRodID(key) || !LevelEditorLayer::get()) {
            return GameObject::createWithKey(key);
        }

        PulseRodGameObject* obj = new PulseRodGameObject();
        const char* frame = ObjectToolbox::sharedState()->intKeyToFrame(key);

        if (obj && obj->init(frame)) {
            obj->autorelease();
            obj->m_objectID = key;

            return obj;
        }

        CC_SAFE_DELETE(obj);
        return nullptr;
    }
};

class $modify(SPRBLevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        ObjectEventListener objectListener;
        std::vector<PulseRodGameObject*> pulseRods;
        short pulseRodIndex = 0;
    };

    $override
    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        generateRodIndex();

        m_fields->objectListener.bind([&](ObjectEvent* event) {
            if (!isPulseRod(event->object)) return ListenerResult::Propagate;

            if (event->isAdded) {
                addPulseRodBall(static_cast<PulseRodGameObject*>(event->object));
            } else {
                removePulseRodBall(static_cast<PulseRodGameObject*>(event->object));
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

        for (const auto& rod : m_fields->pulseRods) {
            rod->m_ball->setDisplayFrame(CCSpriteFrameCache::get()->spriteFrameByName(frame.c_str()));
        }
    }

    void addPulseRodBall(PulseRodGameObject* object) {
        std::string frame = fmt::format("rod_ball_{:02}_001.png", m_fields->pulseRodIndex);
        CCSprite* ball = CCSprite::createWithSpriteFrameName("rod_ball_01_001.png");

        m_fields->pulseRods.push_back(object);
        object->m_ball = ball;
    }

    void removePulseRodBall(PulseRodGameObject* object) {
        auto& rods = m_fields->pulseRods;

        auto it = std::find(rods.begin(), rods.end(), object);
        if (it == rods.end()) return;

        object->m_ball->removeFromParent();
        object->m_ball = nullptr;
        rods.erase(it);
    }

    void generateRodIndex() {
        m_fields->pulseRodIndex = rand() % 3 + 1;
    }
};

void ie::updatePulseRodBalls(LevelEditorLayer* lel, float audioScale) {
    auto& pulseRods = static_cast<SPRBLevelEditorLayer*>(lel)->m_fields->pulseRods;

    if (audioScale == -1.f) audioScale = 1.f;

    for (const auto& rod : pulseRods) {
        auto ball = rod->m_ball;
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
