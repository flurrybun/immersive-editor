#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "PlaytestEvent.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(LevelEditorLayer) {
    $override
    void onPlaytest() {
        LevelEditorLayer::onPlaytest();

        PlaytestEvent(PlaybackMode::Playing).post();
    }

    $override
    void onStopPlaytest() {
        LevelEditorLayer::onStopPlaytest();

        PlaytestEvent(PlaybackMode::Not).post();
    }

    $override
    void onResumePlaytest() {
        LevelEditorLayer::onResumePlaytest();

        PlaytestEvent(PlaybackMode::Playing).post();
    }
};

class $modify(EditorUI) {
    $override
    void onPlaytest(CCObject* sender) {
        EditorUI::onPlaytest(sender);

        // LevelEditorLayer::onPausePlaytest is inlined on windows

        if (m_editorLayer->m_playbackMode == PlaybackMode::Paused) {
            PlaytestEvent(PlaybackMode::Paused).post();
        }
    }
};
