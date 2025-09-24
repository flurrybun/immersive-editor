#include <Geode/modify/LevelEditorLayer.hpp>
#include "ObjectEvent.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(LevelEditorLayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("LevelEditorLayer::init", Priority::LatePost);
    }

    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        for (const auto& object : CCArrayExt<GameObject*>(m_objects)) {
            ObjectEvent(object, true).post();
        }

        return true;
    }

    $override
    CCArray* createObjectsFromString(gd::string const& p0, bool p1, bool p2) {
        CCArray* ret = LevelEditorLayer::createObjectsFromString(p0, p1, p2);

        for (const auto& object : CCArrayExt<GameObject*>(ret)) {
            ObjectEvent(object, true).post();
        }

        return ret;
    }

    $override
    GameObject* createObject(int p0, CCPoint p1, bool p2) {
        GameObject* object = LevelEditorLayer::createObject(p0, p1, p2);

        ObjectEvent(object, true).post();

        return object;
    }

    $override
    void removeObject(GameObject* object, bool p1) {
        LevelEditorLayer::removeObject(object, p1);

        ObjectEvent(object, false).post();
    }
};
