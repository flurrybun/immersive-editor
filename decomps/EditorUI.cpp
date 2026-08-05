#include <Geode/Geode.hpp>
using namespace geode::prelude;

class _EditorUI : public EditorUI {
public:
    void _selectObjects(CCArray* objects, bool ignoreFilter);
};

void _EditorUI::_selectObjects(CCArray* objects, bool ignoreFilter) {
    if (!objects || objects->count() == 0) return;

    auto gm = GameManager::get();

    if (
        !ignoreFilter &&
        gm->getGameVariable(GameVar::SelectFilter) &&
        gm->getIntGameVariable(GameVar::DeleteFilter) == 3
    ) {
        int filterID = gm->getIntGameVariable(GameVar::CustomDeleteFilter);
        int i = 0;

        while (i < objects->count()) {
            GameObject* object = static_cast<GameObject*>(objects->objectAtIndex(i));
            if (object->m_objectID == filterID) continue;

            objects->removeObjectAtIndex(i);
        }
    }

    if (objects->count() == 0) return;

    int filterGroupID = gm->getIntGameVariable(GameVar::GroupIDFilter);

    if (!ignoreFilter && filterGroupID != 0) {
        int i = 0;

        while (i < objects->count()) {
            GameObject* object = static_cast<GameObject*>(objects->objectAtIndex(i));
            if (object->belongsToGroup(filterGroupID)) continue;

            objects->removeObjectAtIndex(i);
        }
    }

    if (objects->count() == 0) return;

    int filterColorID = gm->getIntGameVariable(GameVar::ColorIDFilter);

    if (!ignoreFilter && filterColorID != 0) {
        int i = 0;

        while (i < objects->count()) {
            GameObject* object = static_cast<GameObject*>(objects->objectAtIndex(i));
            if (
                object->getMainColorMode() == filterColorID ||
                object->getSecondaryColorMode() == filterColorID
            ) continue;

            objects->removeObjectAtIndex(i);
        }
    }

    if (objects->count() == 0) return;

    stopActionByTag(124);

    if (m_selectedObject && !objects->containsObject(m_selectedObject)) {
        if (m_selectedObjects->count() > 0) {
            objects->addObject(m_selectedObject);
        } else {
            m_selectedObjects->addObject(m_selectedObject);
            m_selectedObject->selectObject({ 0, 255, 0 });
        }

        m_canActivateControls = true;
    }

    // i guess using a std::unordered_set is simply too convenient and performant

    CCDictionary* linkedDict = nullptr;
    CCNode* dummyNode = nullptr;

    m_selectedObject = nullptr;

    for (auto object : CCArrayExt<GameObject, false>(objects)) {
        if (m_selectedObjects->containsObject(object)) continue;

        m_selectedObjects->addObject(object);
        object->selectObject({ 0, 255, 0 });

        m_canActivateControls = true;

        if (m_stickyControlsEnabled && !m_linkControlsDisabled && object->m_linkedGroup > 0) {
            if (!linkedDict) {
                linkedDict = CCDictionary::create();
                dummyNode = CCNode::create();
            }

            linkedDict->setObject(dummyNode, object->m_linkedGroup);
        }
    }

    if (objects->count() == 1 && m_selectedObjects->count() == 1) {
        GameObject* object = static_cast<GameObject*>(objects->firstObject());
        CCArray* group = m_editorLayer->getStickyGroup(object->m_linkedGroup);

        if (
            !m_stickyControlsEnabled ||
            m_linkControlsDisabled ||
            object->m_linkedGroup <= 0 ||
            !group ||
            group->count() <= 1
        ) {
            selectObject(object, false);
        }
    }

    if (m_selectedObjects->count() > 0 && linkedDict) {
        m_findSnap = true;

        CCArray* keys = linkedDict->allKeys();

        for (auto key : CCArrayExt<CCInteger, false>(keys)) {
            int linkedGroup = key->getValue();
            CCArray* group = m_editorLayer->getStickyGroup(linkedGroup);
            if (!group) return;

            for (auto object : CCArrayExt<GameObject, false>(group)) {
                if (m_selectedObjects->containsObject(object)) continue;

                m_selectedObjects->addObject(object);
                m_selectedObject->selectObject({ 0, 255, 0 });
            }
        }
    }

    checkLiveColorSelect();
}
