#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "PlaytestEvent.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(LevelEditorLayer) {
    $override
    void onPlaytest() {
        bool wasPlaying = m_playbackMode == PlaybackMode::Playing;
        LevelEditorLayer::onPlaytest();

        if (!wasPlaying) {
            PlaytestEvent().send(PlaybackMode::Playing);
        }
    }

    $override
    void onStopPlaytest() {
        bool wasNot = m_playbackMode == PlaybackMode::Not;

        LevelEditorLayer::onStopPlaytest();

        if (!wasNot) {
            PlaytestEvent().send(PlaybackMode::Not);
        }
    }
};

// onPausePlaytest and onResumePlaytest are inlined on windows

class $modify(EditorUI) {
    $override
    void onPlaytest(CCObject* sender) {
        bool wasPaused = m_editorLayer->m_playbackMode == PlaybackMode::Paused;

        if (!m_isPaused && wasPaused) {
            PlaytestEvent().send(PlaybackMode::Playing);
        }

        EditorUI::onPlaytest(sender);

        if (!wasPaused && m_editorLayer->m_playbackMode == PlaybackMode::Paused) {
            PlaytestEvent().send(PlaybackMode::Paused);
        }
    }
};
