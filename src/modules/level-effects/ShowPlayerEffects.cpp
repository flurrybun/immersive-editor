#include "core/SettingManager.hpp"
#include "util/Editor.hpp"
#include "util/Temporary.hpp"

#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(PlayerObject) {
    $register_hooks("show-player-effects");

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

class $modify(SPEGJBaseGameLayer, GJBaseGameLayer) {
    $register_hooks("show-player-effects");

    $override
    void toggleDualMode(GameObject* object, bool dual, PlayerObject* player, bool noEffects) {
        // ⏺️ add player 2 particles

        bool changed = m_gameState.m_isDualMode != dual;

        GJBaseGameLayer::toggleDualMode(object, dual, player, noEffects);

        if (!changed || !m_gameState.m_isDualMode || !ie::inEditor()) return;

        toggleAllParticles(m_player2, true);
    }

    void toggleAllParticles(PlayerObject* player, bool show) {
        // decomp of PlayerObject::addAllParticles but toggles instead of just adding

        toggleParticle(player, show, player->m_playerGroundParticles, 39);
        toggleParticle(player, show, player->m_ufoClickParticles, 39);
        toggleParticle(player, show, player->m_dashParticles, 39);
        toggleParticle(player, show, player->m_robotBurstParticles, 39);
        toggleParticle(player, show, player->m_trailingParticles, 39);
        toggleParticle(player, show, player->m_shipClickParticles, 39);

        toggleParticle(player, show, player->m_vehicleGroundParticles, 61);
        toggleParticle(player, show, player->m_landParticles0, 61);
        toggleParticle(player, show, player->m_landParticles1, 61);

        toggleParticle(player, show, player->m_swingBurstParticles1, 39);
        toggleParticle(player, show, player->m_swingBurstParticles2, 39);
    }

    void toggleParticle(PlayerObject* player, bool show, CCParticleSystemQuad* particle, int zOrder) {
        if (show) {
            player->m_parentLayer->addChild(particle, zOrder);
        } else {
            particle->removeFromParent();
        }
    }
};

$on_enable("show-player-effects") {
    if (auto mod = Loader::get()->getLoadedMod("nytelyte.wave_trail_drag_fix")) {
        mod->setSavedValue("show-in-editor", true);
    }

    auto bgl = modify_cast<SPEGJBaseGameLayer*>(ctx.m_lel);

    bgl->toggleAllParticles(bgl->m_player1, true);
    bgl->toggleAllParticles(bgl->m_player2, true);
}

$on_disable("show-player-effects") {
    if (auto mod = Loader::get()->getLoadedMod("nytelyte.wave_trail_drag_fix")) {
        mod->setSavedValue("show-in-editor", false);
    }

    auto bgl = modify_cast<SPEGJBaseGameLayer*>(ctx.m_lel);

    bgl->toggleAllParticles(bgl->m_player1, false);
    bgl->toggleAllParticles(bgl->m_player2, false);
}
