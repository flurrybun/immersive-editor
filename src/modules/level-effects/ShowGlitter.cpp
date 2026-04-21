#include "ShowGlitter.hpp"
#include "core/SettingManager.hpp"
#include "events/PlaytestEvent.hpp"

#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/utils/VMTHookManager.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

$bind_setting(g_showGlitter, "show-glitter");

class $modify(SGLevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        ListenerHandle playtestListener;
        bool glitterVisible = false;
        bool bgEffectEnabled = true;
    };

    $register_hooks("show-glitter");

    $override
    void toggleGlitter(bool visible) {
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
};

$on_enable("show-glitter") {
    LevelEditorLayer* lel = ctx.m_lel;

    ctx.addVirtualHook<
        ResolveC<SGLevelEditorLayer>::func(&SGLevelEditorLayer::toggleGlitter)
    >(lel, "LevelEditorLayer::toggleGlitter");

    ctx.addEventListener(PlaytestEvent(), [lel](PlaytestMode mode) {
        lel->m_glitterParticles->setVisible(!mode.isNot());
    });

    lel->m_glitterParticles = CCParticleSystemQuad::create("glitterEffect.plist", false);

    lel->m_objectLayer->addChild(lel->m_glitterParticles);
    lel->m_glitterParticles->setVisible(false);
    lel->m_glitterParticles->stopSystem();

    ccColor4F color = to4F(to4B(lel->m_player1->m_playerColor1));
    lel->m_glitterParticles->setStartColor(color);
    lel->m_glitterParticles->setEndColor(color);

    lel->m_glitterParticles->setPositionType(kCCPositionTypeRelative);
}

$on_disable("show-glitter") {
    LevelEditorLayer* lel = ctx.m_lel;

    if (lel->m_glitterParticles) {
        lel->m_glitterParticles->removeFromParent();
        lel->m_glitterParticles = nullptr;
    }
}

void toggleBGEffectVisibility(bool visible) {
    if (!g_showGlitter) return;

    auto lel = LevelEditorLayer::get();
    if (!lel) return;

    static_cast<SGLevelEditorLayer*>(lel)->m_fields->bgEffectEnabled = visible;
    lel->toggleGlitter(static_cast<SGLevelEditorLayer*>(lel)->m_fields->glitterVisible);
}
