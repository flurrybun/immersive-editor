#include "core/UpdateVisibility.hpp"
#include "util/Utils.hpp"

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

namespace ie {
    void updateVisibility(LevelEditorLayer* lel, float dt);
    void postUpdateVisibility(LevelEditorLayer* lel);

    static float g_audioScale;
    static float g_cameraXCenter;
    static ie::GlowContext g_glowContext;
    static bool g_flipping;
}

void ie::updateVisibility(LevelEditorLayer* lel, float dt) {
    g_audioScale = ie::preUpdateAudioScale(lel, dt);
    g_cameraXCenter = ie::preUpdateFadeAndEnter(lel);
    g_glowContext = ie::preUpdateGlow(lel);
    g_flipping = ie::preUpdateMirrorEffect(lel);

    // using a range-based for loop on m_activeObjects can crash due to a use-after-free
    // place 2 objects, undo 2x, place an object -> crash

    for (int i = 0; i < lel->m_activeObjectsCount; i++) {
        GameObject* object = lel->m_activeObjects[i];

        ie::updateAudioScale(lel, object, g_audioScale);
        ie::updateGradientTrigger(object);
        ie::updateParticleIcon(lel, object);
        ie::updateFadeAndEnter(lel, object, g_cameraXCenter);
        ie::updateMirrorEffect(lel, object, g_flipping);
        ie::updateGlow(lel, object, g_glowContext);
        ie::updateObjectParticle(lel, object);
        ie::updateDetailColorOpacity(lel, object);
        ie::updateSelectPreview(lel, object);
    }
}

void ie::postUpdateVisibility(LevelEditorLayer* lel) {
    ie::postUpdateSelectPreview(lel);
    ie::updatePortalBacks(lel);
    ie::updatePulseRodBalls(lel, g_audioScale);
}

class $modify(GJBaseGameLayer) {
    $override
    void updateEnterEffects(float dt) {
        // first function called after the m_activeObjects loop

        if (ie::inEditor()) {
            auto lel = static_cast<LevelEditorLayer*>(static_cast<GJBaseGameLayer*>(this));
            ie::updateVisibility(lel, dt);
        }

        GJBaseGameLayer::updateEnterEffects(dt);
    }
};

class $modify(LevelEditorLayer) {
    $override
    void updateVisibility(float dt) {
        LevelEditorLayer::updateVisibility(dt);

        ie::postUpdateVisibility(this);
    }
};
