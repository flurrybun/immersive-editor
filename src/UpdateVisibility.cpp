#include <Geode/modify/LevelEditorLayer.hpp>
#include "UpdateVisibility.hpp"
#include "misc/Utils.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(LevelEditorLayer) {
    $override
    void updateVisibility(float dt) {
        LevelEditorLayer::updateVisibility(dt);

        float audioScale = ie::preUpdateAudioScale(this, dt);
        float cameraXCenter = ie::preUpdateFadeAndEnter(this);
        ie::GlowContext glowContext = ie::preUpdateGlow(this);
        bool flipping = ie::preUpdateMirrorEffect(this);

        // using a range-based for loop on m_activeObjects can crash due to a use-after-free
        // place 2 objects, undo 2x, place an object -> crash

        for (size_t i = 0; i < m_activeObjectsCount; i++) {
            GameObject* object = m_activeObjects[i];

            ie::updateAudioScale(this, object, audioScale);
            ie::updateGradientTrigger(object);
            ie::updateParticleIcon(this, object);
            ie::updateFadeAndEnter(this, object, cameraXCenter);
            ie::updateMirrorEffect(this, object, flipping);
            ie::updateGlow(this, object, glowContext);
            ie::updateObjectParticle(this, object);
        }

        ie::updatePortalBacks(this);
        ie::updatePulseRodBalls(this, audioScale);
    }
};
