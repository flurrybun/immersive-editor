#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/utils/VMTHookManager.hpp>
#include "ShowGlitter.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(SGLevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        bool glitterVisible = false;
        bool bgEffectEnabled = true;
    };

    $override
    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        auto hook = VMTHookManager::get().addHook<
            ResolveC<SGLevelEditorLayer>::func(&SGLevelEditorLayer::toggleGlitter)
        >(this, "LevelEditorLayer::toggleGlitter");

        m_glitterParticles = CCParticleSystemQuad::create("glitterEffect.plist", false);

        m_objectLayer->addChild(m_glitterParticles);
        m_glitterParticles->setVisible(false);
        m_glitterParticles->stopSystem();

        ccColor4F color = to4F(to4B(m_player1->m_playerColor1));
        m_glitterParticles->setStartColor(color);
        m_glitterParticles->setEndColor(color);

        m_glitterParticles->setPositionType(kCCPositionTypeRelative);

        return true;
    }

    $override
    void toggleGlitter(bool visible) {
        log::info("toggleGlitter {}", visible);

        if (GameManager::get()->m_performanceMode) return;

        m_fields->glitterVisible = visible;

        if (visible && m_fields->bgEffectEnabled) {
            m_glitterParticles->resumeSystem();
        } else {
            m_glitterParticles->stopSystem();
        }
    }

    $override
    void postUpdate(float dt) {
        LevelEditorLayer::postUpdate(dt);

        CCSize winSize = CCDirector::get()->getWinSize();
        CCPoint cameraCenter = winSize * 0.5f / m_gameState.m_cameraZoom;

        m_glitterParticles->setPosition(cameraCenter + m_gameState.m_cameraPosition);
    }

    $override
    void onPlaytest() {
        LevelEditorLayer::onPlaytest();

        m_glitterParticles->setVisible(true);
    }

    $override
    void onStopPlaytest() {
        LevelEditorLayer::onStopPlaytest();

        m_glitterParticles->stopSystem();
        m_glitterParticles->setVisible(false);
    }
};

void toggleBGEffectVisibility(bool visible) {
    auto lel = LevelEditorLayer::get();
    if (!lel) return;

    static_cast<SGLevelEditorLayer*>(lel)->m_fields->bgEffectEnabled = visible;
    lel->toggleGlitter(static_cast<SGLevelEditorLayer*>(lel)->m_fields->glitterVisible);
}
