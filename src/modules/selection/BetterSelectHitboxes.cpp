#include <Geode/modify/LevelEditorLayer.hpp>
#include "Selection.hpp"
#include "../misc/Utils.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(BSHLevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        std::deque<WeakRef<GameObject>> cycledObjects;
    };

    $override
    CCArray* objectsAtPosition(CCPoint position) {
        std::vector<GameObject*> objects = ie::objectsAtPosition(this, position, false);

        CCArray* ret = CCArray::create();

        for (const auto& object : objects) {
            ret->addObject(object);
        }

        return ret;
    }

    $override
    GameObject* objectAtPosition(CCPoint position) {
        return ie::objectAtPosition(this, position, false);
    }

    $override
    CCArray* objectsInRect(CCRect rect, bool ignoreGroups) {
        // only false when called from EditorUI::selectObjectsInRect
        if (ignoreGroups) return LevelEditorLayer::objectsInRect(rect, ignoreGroups);

        // unlike with objectsAtPosition, m_activeObjects can't be solely relied on, since it's possible for
        // the rect to be partially off-screen. however, relying solely on sections also doesn't work
        // for objects with a scale larger than around 15x. so, a two-pass check is used:

        // 1. check all active objects

        CCArray* objects = CCArray::create();
        std::unordered_set<GameObject*> seen;

        auto checkObject = [&](GameObject* object) {
            if (!seen.insert(object).second) return;
            if (!ie::isObjectLayerVisible(object, this)) return;

            ie::SelectionBox box = ie::SelectionBox::fromObject(this, object);
            if (!box.intersectsRect(rect)) return;

            objects->addObject(object);
        };

        for (int i = 0; i < m_activeObjectsCount; i++) {
            GameObject* object = m_activeObjects[i];
            checkObject(object);
        }

        // 2. check objects in sections
        // mostly decompiled from LevelEditorLayer::objectsInRect, but with a different intersection check

        for (GameObject* object : ie::objectsInSections(this, rect)) {
            checkObject(object);
        }

        return objects;
    }
};

std::vector<GameObject*> ie::objectsInSections(LevelEditorLayer* lel, const CCRect& rect) {
    std::vector<GameObject*> objects;

    if (lel->m_sections.empty()) return objects;

    auto sectionX = [&](float x) -> int {
        if (x <= 0.f) return 0.f;
        return lel->m_sectionXFactor * std::min(x, 1e7f);
    };

    auto sectionY = [&](float y) -> int {
        if (y <= 0.f) return 0.f;
        return lel->m_sectionYFactor * std::min(y, 1e7f);
    };

    int xStart = std::max(0, sectionX(rect.origin.x) - 1);
    int xEnd = std::min(sectionX(rect.origin.x + rect.size.width) + 1, (int)lel->m_sections.size() - 1);

    int yStart = std::max(0, sectionY(rect.origin.y) - 1);
    int yEnd = sectionY(rect.origin.y + rect.size.height) + 1;

    for (int xi = xStart; xi <= xEnd; xi++) {
        auto xBucket = lel->m_sections[xi];
        if (!xBucket) continue;

        int bucketYMax = (int)xBucket->size() - 1;
        int bucketYEnd = std::min(yEnd, bucketYMax);

        for (int yi = yStart; yi <= bucketYEnd; yi++) {
            auto yBucket = (*xBucket)[yi];
            if (!yBucket) continue;

            int count = (*lel->m_sectionSizes[xi])[yi];

            for (int i = 0; i < count; i++) {
                objects.push_back((*yBucket)[i]);
            }
        }
    }

    return objects;
}

std::vector<GameObject*> ie::objectsAtPosition(LevelEditorLayer* lel, const CCPoint& position, bool selecting) {
    // this function normally uses sections and obb2d, which is perhaps more performant and works off-screen
    // however it should be safe to assume this will only ever be used for on-screen objects
    // and relying on sections doesn't work for heavily upscaled objects with large bounding boxes

    std::deque<WeakRef<GameObject>>& cycledObjects = static_cast<BSHLevelEditorLayer*>(lel)->m_fields->cycledObjects;
    std::vector<GameObject*> objects;

    CCRect rect = CCRect(position.x, position.y, 0.f, 0.f);

    for (GameObject* object : ie::objectsInSections(lel, rect)) {
        if (!ie::isObjectLayerVisible(object, lel)) continue;

        ie::SelectionBox box = ie::SelectionBox::fromObject(lel, object);
        if (!box.containsPoint(position, true)) continue;

        objects.push_back(object);
    }

    if (objects.empty()) {
        cycledObjects.clear();
        return objects;
    }

    // remove objects from duplicate linked groups

    if (ie::isLinkControlsEnabled(lel)) {
        std::vector<int> seenGroups;
        seenGroups.reserve(objects.size());

        std::erase_if(objects, [&](GameObject* obj) {
            if (obj->m_linkedGroup == 0) return false;
            if (ranges::contains(seenGroups, obj->m_linkedGroup)) return true;

            seenGroups.push_back(obj->m_linkedGroup);
            return false;
        });
    }

    // remove objects from cycledObjects that are no longer in consideration

    if (selecting) {
        std::erase_if(cycledObjects, [&](WeakRef<GameObject> objectRef) {
            auto object = objectRef.lock();
            if (!object) return true;

            if (object->m_linkedGroup == 0) {
                return !ranges::contains(objects, object);
            } else {
                return !std::ranges::any_of(objects, [&](GameObject* obj) {
                    return obj->m_linkedGroup == object->m_linkedGroup;
                });
            }
        });
    }

    // make room if necessary

    if (cycledObjects.size() == objects.size() && objects.size() > 0) {
        cycledObjects.pop_back();
    }

    // sort objects by distance to center point, then by bounding box size

    std::sort(objects.begin(), objects.end(), [&](GameObject* a, GameObject* b) {
        float aDist = ccpDistanceSQ(a->getPosition(), position);
        float bDist = ccpDistanceSQ(b->getPosition(), position);

        if (aDist != bDist) return aDist < bDist;

        CCSize aSize = a->getObjectRect().size;
        CCSize bSize = b->getObjectRect().size;

        return aSize.width * aSize.height < bSize.width * bSize.height;
    });

    // the m_cycleIndex stuff is a bit hacky; gd selects the object with the lowest cycle index,
    // so we force which object gd will select by setting the index of all other objects to 100,000.
    // the benefit of this is it still returns all objects at this position, which gd relies on

    std::optional<GameObject*> objectToSelect = ranges::find(objects, [&](GameObject* object) {
        bool selectable = !object->m_isSelected && lel->m_editorUI->canSelectObject(object);
        if (!selectable) return false;

        return !ranges::contains(cycledObjects, [&](WeakRef<GameObject> cycledObjectRef) {
            auto cycledObject = cycledObjectRef.lock();
            return cycledObject && cycledObject == object;
        });
    });

    // changed from "return {};". let's hope this didn't break anything?

    if (!objectToSelect || !*objectToSelect) return objects;

    for (const auto& object : objects) {
        object->m_cycleIndex = object == *objectToSelect ? 0 : 100'000;
    }

    if (selecting) {
        cycledObjects.push_front(*objectToSelect);
    }

    return objects;
}

GameObject* ie::objectAtPosition(LevelEditorLayer* lel, const CCPoint& position, bool selecting) {
    std::vector<GameObject*> objects = ie::objectsAtPosition(lel, position, selecting);

    if (objects.empty()) return nullptr;

    for (const auto& object : objects) {
        if (object->m_cycleIndex == 0) return object;
    }

    return static_cast<GameObject*>(objects[0]);
}
