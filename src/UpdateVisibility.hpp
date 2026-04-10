#pragma once

#include <Geode/Geode.hpp>
using namespace geode::prelude;

namespace ie {
    struct FadeContext {
        float cameraXCenter;
        ccColor3B lbgColor;

        float rightFadeBound;
        float leftFadeBound;
        float leftFadeWidth;
        float rightFadeWidth;
    };

    float preUpdateAudioScale(LevelEditorLayer* lel, float dt);
    ie::FadeContext preUpdateFadeAndEnter(LevelEditorLayer* lel);
    bool preUpdateMirrorEffect(LevelEditorLayer* lel);

    void updateAudioScale(LevelEditorLayer* lel, GameObject* object, float audioScale);
    void updateGradientTrigger(GameObject* object);
    void updateParticleIcon(LevelEditorLayer* lel, GameObject* object);
    void updateFadeAndEnter(LevelEditorLayer* lel, GameObject* object, const FadeContext& fadeContext);
    void updateGlow(LevelEditorLayer* lel, GameObject* object);
    void updateMirrorEffect(LevelEditorLayer* lel, GameObject* object, bool flipping);
    void updateObjectParticle(LevelEditorLayer* lel, GameObject* object);
    void updateDetailColorOpacity(LevelEditorLayer* lel, GameObject* object);
    void updateSelectPreview(LevelEditorLayer* lel, GameObject* object);

    void postUpdateSelectPreview(LevelEditorLayer* lel);
    void updatePortalBacks(LevelEditorLayer* lel);
    void updatePulseRodBalls(LevelEditorLayer* lel, float audioScale);
}
