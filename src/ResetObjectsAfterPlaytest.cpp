#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/GameObject.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

bool s_isStoppingPlaytest = false;

class $modify(LevelEditorLayer) {
    $override
    void onStopPlaytest() {
        if (m_playbackMode != PlaybackMode::Not) {
            s_isStoppingPlaytest = true;
        }

        LevelEditorLayer::onStopPlaytest();
        s_isStoppingPlaytest = false;
    }
};

class $modify(GameObject) {
    $override
    void updateStartValues() {
        // ⏺️ fix objects being flipped the wrong direction when stopping playtest during 2nd half of mirror effect

        if (s_isStoppingPlaytest) {
            setFlipX(m_startFlipX);
            setFlipY(m_startFlipY);
            setRotationX(m_startRotationX);
            setRotationY(m_startRotationY);
        }

        GameObject::updateStartValues();
    }
};
