#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <alphalaneous.good_grid/include/DrawGridAPI.hpp>
#include <alphalaneous.good_grid/include/DrawLayers.hpp>
#include "misc/PlaytestEvent.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

constexpr int ACTION_TAG = 0x8D45C2A4;

class $modify(HEUILevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        PlaytestEventListener playtestListener;
        bool uiVisible = true;
        bool hidePath = false;
        bool showGround = false;
    };

    $override
    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        DrawGridAPI::get().getNode<Ground>("ground").inspect([](Ground& ground) {
            ground.setEnabled(false);
        });

        DrawGridAPI::get().getNode<PositionLines>("position-lines").inspect([](PositionLines& posLines) {
            posLines.setEnabled(false);
        });

        m_fields->playtestListener.bind([&](PlaytestEvent* event) {
            bool isPlaying = event->isPlaying();

            if (isPlaying) {
                m_fields->hidePath = GameManager::get()->getGameVariable("0152");
                GameManager::get()->setGameVariable("0152", true);

                m_fields->showGround = m_showGround;
                m_hideGround = false;
                updateCameraBGArt(m_gameState.m_cameraPosition, m_gameState.m_cameraZoom);
            } else {
                GameManager::get()->setGameVariable("0152", m_fields->hidePath);

                toggleGround(m_fields->showGround);
                if (m_middleground) m_middleground->toggleVisible02(true);
            }

            DrawGridAPI::get().getNode<Bounds>("bounds").inspect([isPlaying](Bounds& bounds) {
                bounds.setEnabled(!isPlaying);
            });

            DrawGridAPI::get().getNode<BPMTriggers>("bpm-triggers").inspect([isPlaying](BPMTriggers& bpmTriggers) {
                bpmTriggers.setEnabled(!isPlaying);
            });

            return ListenerResult::Propagate;
        });

        return true;
    }
};

class $modify(EditorUI) {
    $override
    void updatePlaybackBtn() {
        EditorUI::updatePlaybackBtn();

        bool isPlaying = m_editorLayer->m_playbackMode == PlaybackMode::Playing;
        auto pauseSpr = static_cast<CCSprite*>(m_playtestBtn->getNormalImage());
        auto stopSpr = static_cast<CCSprite*>(m_playtestStopBtn->getNormalImage());

        pauseSpr->setOpacity(isPlaying ? 75 : 255);
        stopSpr->setOpacity(isPlaying ? 75 : 255);

        constexpr float scale = 38.f / 24.f;
        pauseSpr->setScale(isPlaying ? scale : 1.f);
        stopSpr->setScale(isPlaying ? scale : 1.f);

        auto pauseFrame = CCSpriteFrameCache::get()->spriteFrameByName("GJ_pauseBtn_clean_001.png");
        if (isPlaying) pauseSpr->setDisplayFrame(pauseFrame);

        auto stopFrame = CCSpriteFrameCache::get()->spriteFrameByName(
            isPlaying ? "GJ_stopBtn_clean_001.png"_spr : "GJ_stopEditorBtn_001.png"
        );
        stopSpr->setDisplayFrame(stopFrame);

        // maybe this should be done in EditorUI::showUI but betteredit uses that
        // for the hide ui button and i don't want to change that behavior

        auto pauseBtn = getChildByID("settings-menu")->getChildByID("pause-button");
        pauseBtn->setVisible(!isPlaying);
    }
};
