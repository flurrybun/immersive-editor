#include <Geode/modify/LevelEditorLayer.hpp>
#include "misc/ObjectEvent.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(LevelEditorLayer) {
    struct Fields {
        ObjectEventListener objectListener;
    };

    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        m_fields->objectListener.bind([&](ObjectEvent* event) {
            if (event->isAdded) {
                addGuideArt(event->object);
            }

            return ListenerResult::Propagate;
        });

        return true;
    }
};
