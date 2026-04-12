#include "core/SettingManager.hpp"

#include <Geode/modify/LevelTools.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(LevelTools) {
    $register_hooks("fix-performance");

    $override
    static void moveTriggerObjectsToArray(CCArray* objects, CCDictionary* objectsDict, int index) {
        // the original function calls removeObjectAtPosition(0) in a loop, which is horribly inefficient
        // when editing object-heavy levels (tested with hermes by quid) this causes very noticable lag

        CCArray *src = static_cast<CCArray*>(objectsDict->objectForKey(index));

        while (src && src->count() != 0) {
            unsigned int i = 0;
            unsigned int total = src->count();

            for (; i < total; i++) {
                auto object = static_cast<RotateGameplayGameObject*>(src->objectAtIndex(i));

                objects->addObject(object);

                if (object->m_objectID == 2900 && object->m_changeChannel == true) {
                    i++;
                    break;
                }
            }

            if (i < total) {
                CCArray* newSrc = CCArray::createWithCapacity(total - i);

                for (unsigned int j = i; j < total; j++) {
                    newSrc->addObject(src->objectAtIndex(j));
                }

                objectsDict->setObject(newSrc, index);
                src = newSrc;
            } else {
                src = CCArray::create();
                objectsDict->setObject(src, index);
            }

            auto redirectObj = static_cast<RotateGameplayGameObject*>(objects->lastObject());
            index = static_cast<int>(redirectObj->m_targetChannelID);
            src = static_cast<CCArray*>(objectsDict->objectForKey(index));
        }
    }
};
