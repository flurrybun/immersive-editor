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
    float preUpdateFadeAndEnter(LevelEditorLayer* lel);
    GlowContext preUpdateGlow(LevelEditorLayer* lel);
    bool preUpdateMirrorEffect(LevelEditorLayer* lel);

    void updateAudioScale(LevelEditorLayer* lel, GameObject* object, float audioScale);
    void updateGradientTrigger(GameObject* object);
    void updateParticleIcon(LevelEditorLayer* lel, GameObject* object);
    void updateFadeAndEnter(LevelEditorLayer* lel, GameObject* object, float cameraXCenter);
    void updateGlow(LevelEditorLayer* lel, GameObject* object, const GlowContext& context);
    void updateMirrorEffect(LevelEditorLayer* lel, GameObject* object, bool flipping);
    void updateObjectParticle(LevelEditorLayer* lel, GameObject* object);
    void updateDetailColorOpacity(LevelEditorLayer* lel, GameObject* object);

    void updatePortalBacks(LevelEditorLayer* lel);
    void updatePulseRodBalls(LevelEditorLayer* lel, float audioScale);
}
