#pragma once

#include <Geode/loader/Event.hpp>
#include <Geode/Enums.hpp>

using namespace geode::prelude;

// i just think `mode.isPlaying()` looks nicer than `mode == PlaybackMode::Playing`

class PlaytestMode {
    char m_mode;
public:
    PlaytestMode(PlaybackMode mode) : m_mode(static_cast<char>(mode)) {}

    bool isNot() const { return m_mode == 0; }
    bool isPlaying() const { return m_mode == 1; }
    bool isPaused() const { return m_mode == 2; }

    operator PlaybackMode() const { return static_cast<PlaybackMode>(m_mode); }
};

class PlaytestEvent : public Event<PlaytestEvent, bool(PlaytestMode mode)> {
public:
    using Event::Event;
};
