#include <Geode/modify/LevelTools.hpp>
#include "misc/SettingManager.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(LevelTools) {
    $toggle_hooks("fix-performance");

    $override
    static void moveTriggerObjectsToArray(CCArray* objects, CCDictionary* objectsDict, int index) {
        // the original function calls removeObjectAtPosition(0) in a loop, which is horrible inefficient
        // in levels with a lot of objects (tested with hermes by quid) this causes very noticable lag

        CCArray* src = static_cast<CCArray*>(objectsDict->objectForKey(index));

        while (src && src->count() != 0) {
            auto obj = static_cast<RotateGameplayGameObject*>(src->objectAtIndex(0));
            objects->addObject(obj);

            if (obj->m_objectID != 2900 || !obj->m_changeChannel) return;

            src = static_cast<CCArray*>(objectsDict->objectForKey(obj->m_targetChannelID));
        }
    }
};
