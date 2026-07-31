#include "ObjectEvent.hpp"

#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(OELevelEditorLayer, LevelEditorLayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("LevelEditorLayer::init", Priority::LatePost);
    }

    struct Fields {
        bool shouldDelete = true;
    };

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

    // LevelEditorLayer::removeObject is inlined in several places, so we have to do some funny stuff
    // removeSpecial is called in removeObject, but also some other places, so those must be filtered out

    $override
    void removeSpecial(GameObject* object) {
        LevelEditorLayer::removeSpecial(object);

        if (m_fields->shouldDelete) ObjectEvent().send(object, false);
    }
};

class $modify(EditorUI) {
    $override
    void createLoop() {
        static_cast<OELevelEditorLayer*>(m_editorLayer)->m_fields->shouldDelete = false;
        EditorUI::createLoop();
        static_cast<OELevelEditorLayer*>(m_editorLayer)->m_fields->shouldDelete = true;
    }

    $override
    void createNewKeyframeAnim() {
        static_cast<OELevelEditorLayer*>(m_editorLayer)->m_fields->shouldDelete = false;
        EditorUI::createNewKeyframeAnim();
        static_cast<OELevelEditorLayer*>(m_editorLayer)->m_fields->shouldDelete = true;
    }

    $override
    void onCreateObject(int id) {
        static_cast<OELevelEditorLayer*>(m_editorLayer)->m_fields->shouldDelete = false;
        EditorUI::onCreateObject(id);
        static_cast<OELevelEditorLayer*>(m_editorLayer)->m_fields->shouldDelete = true;
    }
};
