#include "events/ObjectEvent.hpp"

#include <Geode/modify/LevelEditorLayer.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(LevelEditorLayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("LevelEditorLayer::init", Priority::LatePost);
    }

    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        for (const auto& object : CCArrayExt<GameObject*>(m_objects)) {
            ObjectEvent().send(object, true);
        }

        return true;
    }

    $override
    CCArray* createObjectsFromString(gd::string const& objString, bool dontCreateUndo, bool dontShowMaxWarning) {
        // note: dontCreateUndo is always true, so on windows 2.2081 it's been optimized out
        // so garbage data is passed in its place

        CCArray* ret = LevelEditorLayer::createObjectsFromString(objString, dontCreateUndo, dontShowMaxWarning);
        if (dontShowMaxWarning) return ret;

        for (const auto& object : CCArrayExt<GameObject*>(ret)) {
            ObjectEvent().send(object, true);
        }

        return ret;
    }

    $override
    GameObject* createObject(int p0, CCPoint p1, bool p2) {
        GameObject* object = LevelEditorLayer::createObject(p0, p1, p2);

        ObjectEvent().send(object, true);

        return object;
    }

    $override
    void removeObject(GameObject* object, bool p1) {
        LevelEditorLayer::removeObject(object, p1);

        ObjectEvent().send(object, false);
    }
};
