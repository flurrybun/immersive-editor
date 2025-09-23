#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/utils/VMTHookManager.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(SGELevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        std::vector<Ref<GravityEffectSprite>> gravityEffects;
        int activeGravityEffects = 0;
        int gravityEffectIndex = 0;
    };

    $override
    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        auto hook = VMTHookManager::get().addHook<
            ResolveC<SGELevelEditorLayer>::func(&SGELevelEditorLayer::playGravityEffect)
        >(this, "LevelEditorLayer::playGravityEffect");

        m_fields->gravityEffects.reserve(4);

        for (int i = 0; i < 4; i++) {
            m_fields->gravityEffects.push_back(GravityEffectSprite::create());
        }

        return true;
    }

    void playGravityEffect(bool upsideDown) {
        bool disabledGravityEffects = GameManager::get()->getGameVariable("0072");
        if (GameManager::get()->m_performanceMode || disabledGravityEffects) return;

        size_t totalGravityEffects = m_fields->gravityEffects.size();
        if (m_fields->activeGravityEffects >= totalGravityEffects) return;

        auto effect = m_fields->gravityEffects[m_fields->gravityEffectIndex];

        m_fields->gravityEffectIndex++;
        m_fields->gravityEffectIndex %= totalGravityEffects;

        m_fields->activeGravityEffects++;

        bool sideways = m_player1->m_isSideways;

        CCSize winSize = CCDirector::get()->getWinSize();
        CCPoint bottom, top;

        if (sideways) {
            top = ccp(-95.f, winSize.height * 0.5f);
            bottom = ccp(winSize.width + 95.f, winSize.height * 0.5f);
        } else {
            top = ccp(winSize.width * 0.5f, winSize.height + 95.f);
            bottom = ccp(winSize.width * 0.5f, -95.f);
        }

        CCPoint startPos = upsideDown ? bottom : top;
        CCPoint endPos = upsideDown ? top : bottom;

        effect->setPosition(startPos);
        effect->setRotation(sideways ? 90.f : 0.f);
        effect->setFlipY(!upsideDown);
        effect->setVisible(true);

        effect->updateSpritesColor(upsideDown ? ccc3(255, 255, 0) : ccc3(0, 255, 255));

        addChild(effect);

        effect->runAction(
            CCSequence::create(
                CCMoveTo::create(0.4f, endPos),
                CallFuncExt::create([this, effect] {
                    m_fields->activeGravityEffects--;

                    effect->setVisible(false);
                    effect->removeFromParent();
                }),
                nullptr
            )
        );
    }
};

class $modify(PlayerObject) {
    $override
    void ringJump(RingObject* ring, bool p1) {
        // orbs check if m_playEffects is true before calling playGravityEffect
        // even though this check is completely redundant

        PlayerObject::ringJump(ring, p1);

        GameObjectType type = ring->getType();
        bool isGravityRing =
            type == GameObjectType::GravityRing ||
            type == GameObjectType::GreenRing ||
            type == GameObjectType::GravityDashRing;

        if (LevelEditorLayer::get() && isGravityRing) {
            m_gameLayer->playGravityEffect(m_isUpsideDown);
        }
    }
};
