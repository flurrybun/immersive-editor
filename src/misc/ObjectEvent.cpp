#include <Geode/modify/LevelEditorLayer.hpp>
#include "ObjectEvent.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(LevelEditorLayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("LevelEditorLayer::init", Priority::LatePost);
        (void)self.setHookPriority("LevelEditorLayer::createObjectsFromString", Priority::EarlyPre);

        // why the hook for createObjectsFromString?
        // well clean startpos sucks and sets p1 and p2 to true for no reason
        // https://github.com/blueblock6/CleanStartpos/blob/c1a9c07ef765647240959da0c65cd565c8fe1753/src/main.cpp#L588
    }

    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        for (const auto& object : CCArrayExt<GameObject*>(m_objects)) {
            ObjectEvent(object, true).post();
        }

        return true;
    }

    $override
    CCArray* createObjectsFromString(gd::string const& objString, bool dontCreateUndo, bool dontShowMaxWarning) {
        CCArray* ret = LevelEditorLayer::createObjectsFromString(objString, dontCreateUndo, dontShowMaxWarning);
        if (dontCreateUndo || dontShowMaxWarning) return ret;

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
