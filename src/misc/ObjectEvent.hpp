#pragma once

#include <Geode/loader/Event.hpp>

using namespace geode::prelude;

class ObjectEvent : public Event {
public:
    bool isAdded;
    GameObject* object;

    ObjectEvent(GameObject* object, bool isAdded) : object(object), isAdded(isAdded) {}
};

using ObjectEventListener = EventListener<EventFilter<ObjectEvent>>;
