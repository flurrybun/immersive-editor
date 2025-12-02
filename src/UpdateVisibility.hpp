#pragma once

#include <Geode/Geode.hpp>
using namespace geode::prelude;

namespace ie {
    struct GlowContext {
        float rightFadeBound;
        float leftFadeBound;
        float leftFadeWidth;
        float rightFadeWidth;
        ccColor3B lbgColor;
    };

    float preUpdateAudioScale(LevelEditorLayer* lel, float dt);
    GlowContext preUpdateGlow(LevelEditorLayer* lel);

    void updateAudioScale(LevelEditorLayer* lel, GameObject* object, float audioScale);
    void updateGradientTrigger(GameObject* object);
    void updateParticleIcon(LevelEditorLayer* lel, GameObject* object);
    void updateGlow(LevelEditorLayer* lel, GameObject* object, const GlowContext& context);
    void updateObjectParticle(LevelEditorLayer* lel, GameObject* object);

    void updatePortalBacks(LevelEditorLayer* lel);
    void updatePulseRodBalls(LevelEditorLayer* lel, float audioScale);
}
