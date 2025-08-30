#pragma once

#include <Geode/loader/Event.hpp>
#include <Geode/Enums.hpp>

using namespace geode::prelude;

class PlaytestEvent : public Event {
public:
    PlaybackMode mode;

    bool isPlaying() const {
        return mode == PlaybackMode::Playing;
    }

    PlaytestEvent(PlaybackMode mode) : mode(mode) {}
};

using PlaytestEventListener = EventListener<EventFilter<PlaytestEvent>>;
