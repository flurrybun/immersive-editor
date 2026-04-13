#include "core/SettingManager.hpp"
#include "events/PlaytestEvent.hpp"

#include <alphalaneous.good_grid/include/DrawGridAPI.hpp>
#include <alphalaneous.good_grid/include/DrawLayers.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

constexpr int ACTION_TAG = hash("ninkaz loves you <3");

$bind_setting(g_hideGridLines, "hide-grid-lines");
$bind_setting(g_hideEditorTrail, "hide-editor-trail");
$bind_setting(g_hideTriggers, "hide-triggers");
$bind_setting(g_showGround, "show-ground");
$bind_setting(g_autoHidePlaytestButtons, "auto-hide-playtest-buttons");
$bind_setting(g_transparentPlaytestButtons, "transparent-playtest-buttons");

class $modify(HEUILevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        ListenerHandle playtestListener;
        bool uiVisible = true;
        bool hidePath = false;
        bool showGround = false;
    };

    $override
    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        DrawGridAPI::get().getNode<Ground>("ground").inspect([](Ground& ground) {
            ground.setEnabled(g_hideGridLines);
        });

        DrawGridAPI::get().getNode<PositionLines>("position-lines").inspect([](PositionLines& posLines) {
            posLines.setEnabled(g_hideGridLines);
        });

        m_fields->playtestListener = PlaytestEvent().listen([this](PlaytestMode mode) {
            bool isPlaying = mode.isPlaying();

            if (isPlaying) {
                m_fields->hidePath = GameManager::get()->getGameVariable("0152");
                if (g_hideEditorTrail) GameManager::get()->setGameVariable("0152", true);

                m_fields->showGround = m_showGround;
                m_hideGround = false;
                updateCameraBGArt(m_gameState.m_cameraPosition, m_gameState.m_cameraZoom);
            } else {
                GameManager::get()->setGameVariable("0152", m_fields->hidePath);

                toggleGround(m_fields->showGround);
                if (m_middleground) m_middleground->toggleVisible02(true);
            }

            DrawGridAPI::get().getNode<Bounds>("bounds").inspect([isPlaying](Bounds& bounds) {
                bounds.setEnabled(g_hideGridLines ? !isPlaying : true);
            });

            DrawGridAPI::get().getNode<BPMTriggers>("bpm-triggers").inspect([isPlaying](BPMTriggers& bpmTriggers) {
                bpmTriggers.setEnabled(g_hideTriggers ? !isPlaying : true);
            });

            if (!g_autoHidePlaytestButtons) return;

            if (isPlaying) {
                if (!isHoveringUI()) {
                    auto menu = static_cast<CCMenu*>(m_editorUI->getChildByID("playtest-menu"));
                    PlatformToolbox::hideCursor();

                    queueInMainThread([menu]() {
                        menu->setOpacity(0);
                    });

                    m_fields->uiVisible = false;
                }

                schedule(schedule_selector(HEUILevelEditorLayer::updateCursorVisibility));
            } else {
                unschedule(schedule_selector(HEUILevelEditorLayer::updateCursorVisibility));
                resetUIVisibility();
            }
        });

        return true;
    }

    bool isHoveringUI() {
        CCRect rect = m_editorUI->getChildByID("playtest-menu")->boundingBox();
        rect += CCRect(-20, -20, 20, 40); // some extra margin

        CCPoint point = m_editorUI->convertToNodeSpace(getMousePos());

        return rect.containsPoint(point);
    }

    void updateCursorVisibility(float dt) {
        bool visible = isHoveringUI();
        if (visible == m_fields->uiVisible) return;

        setUIVisibility(visible);
    }

    void setUIVisibility(bool visible) {
        m_fields->uiVisible = visible;

        auto menu = m_editorUI->getChildByID("playtest-menu");
        menu->runAction(
            CCFadeTo::create(0.2f, visible ? 255 : 0)
        );

        stopActionByTag(ACTION_TAG);

        if (visible) {
            PlatformToolbox::showCursor();
        } else {
            auto action = runAction(CCSequence::create(
                CCDelayTime::create(0.3f),
                CallFuncExt::create([visible]() {
                    if (visible) PlatformToolbox::showCursor();
                    else PlatformToolbox::hideCursor();
                }),
                nullptr
            ));
            action->setTag(ACTION_TAG);
        }
    }

    void resetUIVisibility() {
        auto menu = static_cast<CCMenu*>(m_editorUI->getChildByID("playtest-menu"));
        menu->stopAllActions();
        menu->setOpacity(255);

        stopActionByTag(ACTION_TAG);
        PlatformToolbox::showCursor();
    }
};

class $modify(EditorUI) {
    $override
    void updatePlaybackBtn() {
        EditorUI::updatePlaybackBtn();

        bool isPlaying = m_editorLayer->m_playbackMode == PlaybackMode::Playing;

        if (g_transparentPlaytestButtons) {
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
        }

        // maybe this should be done in EditorUI::showUI but betteredit uses that
        // for the hide ui button and i don't want to change that behavior

        auto pauseBtn = getChildByID("settings-menu")->getChildByID("pause-button");
        pauseBtn->setVisible(!isPlaying);
    }
};
