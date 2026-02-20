#pragma once

#include <Geode/loader/Event.hpp>

using namespace geode::prelude;

class ObjectEvent : public Event<ObjectEvent, bool(GameObject* object, bool created)> {
public:
    using Event::Event;
};
