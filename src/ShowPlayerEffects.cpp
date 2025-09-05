#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(PlayerObject) {
    $override
    bool init(int player, int ship, GJBaseGameLayer* gameLayer, CCLayer* layer, bool playLayer) {
        // ⏺️ player particles
        // ⏺️ hard streak

        if (!PlayerObject::init(player, ship, gameLayer, layer, playLayer)) return false;

        if (LevelEditorLayer::get()) {
            addAllParticles();
        }

        return true;
    }

    $override
    void updateTimeMod(float p0, bool p1) {
        // ⏺️ speed particles

        if (!LevelEditorLayer::get()) {
            PlayerObject::updateTimeMod(p0, p1);
            return;
        }

        GameManager::get()->m_playLayer = reinterpret_cast<PlayLayer*>(GJBaseGameLayer::get());
        m_playEffects = true;

        PlayerObject::updateTimeMod(p0, p1);

        GameManager::get()->m_playLayer = nullptr;
        m_playEffects = false;
    }

    $override
    void toggleGhostEffect(GhostType type) {
        // ⏺️ ghost trail

        if (!LevelEditorLayer::get()) {
            PlayerObject::toggleGhostEffect(type);
            return;
        }

        GameManager::get()->m_playLayer = reinterpret_cast<PlayLayer*>(GJBaseGameLayer::get());

        PlayerObject::toggleGhostEffect(type);

        GameManager::get()->m_playLayer = nullptr;
    }

    $override
    void startDashing(DashRingObject* p0) {
        // ⏺️ fire effect on activating dash orb

        if (!LevelEditorLayer::get()) {
            PlayerObject::startDashing(p0);
            return;
        }

        GameManager::get()->m_playLayer = reinterpret_cast<PlayLayer*>(GJBaseGameLayer::get());
        m_playEffects = true;

        PlayerObject::startDashing(p0);

        GameManager::get()->m_playLayer = nullptr;
        m_playEffects = false;
    }

    $override
    void activateStreak() {
        // ⏺️ wave trail

        bool prevEE = GameManager::get()->m_editorEnabled;
        GameManager::get()->m_editorEnabled = false;

        PlayerObject::activateStreak();

        GameManager::get()->m_editorEnabled = prevEE;
    }

    $override
    void fadeOutStreak2(float duration) {
        // ⏺️ wave trail fade

        // no clue why but deactivateStreak (inlined on win) sets duration to 0.6 when m_playEffects is false
        // making fade out time in the editor longer than in-game

        if (LevelEditorLayer::get() && duration == 0.6f) {
            duration = 0.2f;
        }

        PlayerObject::fadeOutStreak2(duration);
    }

    $override
    void update(float dt) {
        // ⏺️ wave trail color

        PlayerObject::update(dt);

        if (LevelEditorLayer::get()) {
            m_waveTrail->setColor(m_switchWaveTrailColor ? m_playerColor2 : m_playerColor1);
        }
    }
};

class $modify(LevelEditorLayer) {
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
