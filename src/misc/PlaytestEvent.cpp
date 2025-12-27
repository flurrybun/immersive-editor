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
            PlaytestEvent(PlaybackMode::Playing).post();
        }
    }

    $override
    void onStopPlaytest() {
        bool wasNot = m_playbackMode == PlaybackMode::Not;

        LevelEditorLayer::onStopPlaytest();

        if (!wasNot) {
            PlaytestEvent(PlaybackMode::Not).post();
        }
    }

    $override
    void onResumePlaytest() {
        bool wasPlaying = m_playbackMode == PlaybackMode::Playing;

        LevelEditorLayer::onResumePlaytest();

        if (!wasPlaying) {
            PlaytestEvent(PlaybackMode::Playing).post();
        }
    }
};

class $modify(EditorUI) {
    $override
    void onPlaytest(CCObject* sender) {
        bool wasPaused = m_editorLayer->m_playbackMode == PlaybackMode::Paused;

        EditorUI::onPlaytest(sender);

        // LevelEditorLayer::onPausePlaytest is inlined on windows

        if (!wasPaused && m_editorLayer->m_playbackMode == PlaybackMode::Paused) {
            PlaytestEvent(PlaybackMode::Paused).post();
        }
    }
};
