#include <Geode/modify/LevelEditorLayer.hpp>
#include "misc/ObjectEvent.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(LevelEditorLayer) {
    struct Fields {
        ListenerHandle objectListener;
    };

    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        m_fields->objectListener = ObjectEvent().listen([this](GameObject* object, bool created) {
            if (created) addGuideArt(object);
            return ListenerResult::Propagate;
        });

        return true;
    }
};
