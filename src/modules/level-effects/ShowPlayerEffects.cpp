#include "core/SettingManager.hpp"
#include "util/Editor.hpp"
#include "util/Temporary.hpp"

#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(PlayerObject) {
    $register_hooks("show-player-effects");

    $override
    bool init(int player, int ship, GJBaseGameLayer* gameLayer, CCLayer* layer, bool playLayer) {
        // ⏺️ player particles
        // ⏺️ hard streak

        if (!PlayerObject::init(player, ship, gameLayer, layer, playLayer)) return false;

        if (ie::inEditor()) {
            addAllParticles();
        }

        return true;
    }

    $override
    void flipGravity(bool p0, bool p1) {
        // ⏺️ show hard streak when entering gravity portal

        if (!ie::inEditor()) {
            PlayerObject::flipGravity(p0, p1);
            return;
        }

        ie::withFakePlayLayer([&] {
            PlayerObject::flipGravity(p0, p1);
        });
    }

    $override
    void updateTimeMod(float p0, bool p1) {
        // ⏺️ speed particles

        if (!ie::inEditor()) {
            PlayerObject::updateTimeMod(p0, p1);
            return;
        }

        ie::withFakePlayLayer([&] {
            ie::withTemporary({
                { &m_playEffects, true },
            }, [&] {
                PlayerObject::updateTimeMod(p0, p1);
            });
        });
    }

    $override
    void toggleGhostEffect(GhostType type) {
        // ⏺️ ghost trail

        if (!ie::inEditor()) {
            PlayerObject::toggleGhostEffect(type);
            return;
        }

        ie::withFakePlayLayer([&] {
            PlayerObject::toggleGhostEffect(type);
        });
    }

    $override
    void startDashing(DashRingObject* p0) {
        // ⏺️ fire effect on activating dash orb

        if (!ie::inEditor()) {
            PlayerObject::startDashing(p0);
            return;
        }

        ie::withFakePlayLayer([&] {
            ie::withTemporary({
                { &m_playEffects, true },
            }, [&] {
                PlayerObject::startDashing(p0);
            });
        });
    }

    $override
    void activateStreak() {
        // ⏺️ wave trail

        ie::withTemporary({
            { &GameManager::get()->m_editorEnabled, false }
        }, [&] {
            PlayerObject::activateStreak();
        });
    }

    $override
    void fadeOutStreak2(float duration) {
        // ⏺️ wave trail fade

        // no clue why but deactivateStreak (inlined on win) sets duration to 0.6 when m_playEffects is false
        // making fade out time in the editor longer than in-game

        if (ie::inEditor() && duration == 0.6f) {
            duration = 0.2f;
        }

        PlayerObject::fadeOutStreak2(duration);
    }

    $override
    void update(float dt) {
        // ⏺️ wave trail color

        PlayerObject::update(dt);

        if (ie::inEditor()) {
            m_waveTrail->setColor(m_switchWaveTrailColor ? m_playerColor2 : m_playerColor1);
        }
    }
};

class $modify(LevelEditorLayer) {
    $register_hooks("show-player-effects");

    $override
    bool init(GJGameLevel* p0, bool p1) {
        // ⏺️ wave trail drag fix mod compatibility

        if (!LevelEditorLayer::init(p0, p1)) return false;

        if (auto mod = Loader::get()->getLoadedMod("nytelyte.wave_trail_drag_fix")) {
            mod->setSavedValue("show-in-editor", true);
        }

        return true;
    }
};
