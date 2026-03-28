#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/GameObject.hpp>
#include "UpdateVisibility.hpp"
#include "misc/SelectionBox.hpp"
#include "misc/Utils.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(SPLevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        std::unordered_set<WeakRef<GameObject>> prevHoveredObjects;
        std::deque<WeakRef<GameObject>> cycledObjects;
    };

    $override
    CCArray* objectsAtPosition(CCPoint position) {
        return betterObjectsAtPosition(position, false);
    }

    $override
    CCArray* betterObjectsAtPosition(const CCPoint& position, bool hovering) {
        // this function normally uses sections and obb2d, which is perhaps more performant and works off-screen
        // however it should be safe to assume this will only ever be used for on-screen objects
        // and relying on sections doesn't work for heavily upscaled objects with large bounding boxes

        std::vector<GameObject*> objects;

        for (int i = 0; i < m_activeObjectsCount; i++) {
            GameObject* object = m_activeObjects[i];

            if (!ie::isObjectLayerVisible(object, this)) continue;

            SelectionBox box(this, object, true);
            if (!box.containsPoint(position)) continue;

            objects.push_back(object);
        }

        if (objects.empty()) {
            m_fields->cycledObjects.clear();
            return CCArray::create();
        }

        // remove objects from duplicate linked groups

        if (ie::isLinkControlsEnabled(this)) {
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

        if (!hovering) {
            std::erase_if(m_fields->cycledObjects, [&](WeakRef<GameObject> objectRef) {
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

            // make room if necessary

            if (m_fields->cycledObjects.size() == objects.size() && objects.size() > 0) {
                m_fields->cycledObjects.pop_back();
            }
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
            bool selectable = !object->m_isSelected && m_editorUI->canSelectObject(object);
            if (!selectable) return false;

            return !ranges::contains(m_fields->cycledObjects, [&](WeakRef<GameObject> cycledObjectRef) {
                auto cycledObject = cycledObjectRef.lock();
                return cycledObject && cycledObject == object;
            });
        });

        if (!objectToSelect || !*objectToSelect) {
            return CCArray::create();
        }

        for (const auto& object : objects) {
            object->m_cycleIndex = object == *objectToSelect ? 0 : 100'000;
        }

        if (!hovering) {
            m_fields->cycledObjects.push_front(*objectToSelect);
        }

        // convert to ccarray

        CCArray* ret = CCArray::createWithCapacity(objects.size());

        for (const auto& object : objects) {
            ret->addObject(object);
        }

        return ret;
    }

    $override
    GameObject* objectAtPosition(CCPoint position) {
        return betterObjectAtPosition(position, false);
    }

    $override
    GameObject* betterObjectAtPosition(const CCPoint& position, bool hovering) {
        CCArray* objects = betterObjectsAtPosition(position, hovering);

        for (const auto& object : CCArrayExt<GameObject>(objects)) {
            if (object->m_cycleIndex == 0) return object;
        }

        return static_cast<GameObject*>(objects->firstObject());
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

            SelectionBox box(this, object, false);
            if (!box.intersectsRect(rect)) return;

            objects->addObject(object);
        };

        for (int i = 0; i < m_activeObjectsCount; i++) {
            GameObject* object = m_activeObjects[i];
            checkObject(object);
        }

        // 2. check objects in sections
        // mostly decompiled from LevelEditorLayer::objectsInRect, but with a different intersection check

        if (m_sections.empty()) return objects;

        auto sectionX = [&](float x) -> float {
            if (x <= 0.f) return 0.f;
            return m_sectionXFactor * (x < 1e7f ? x : 1e7f);
        };

        auto sectionY = [&](float y) -> float {
            if (y <= 0.f) return 0.f;
            return m_sectionYFactor * (y < 1e7f ? y : 1e7f);
        };

        float startSectionX = sectionX(rect.origin.x);
        startSectionX = (startSectionX - 1.f >= 0.f) ? (startSectionX - 1.f) : 0.f;

        float endSectionX = sectionX(rect.origin.x + rect.size.width);
        float maxSectionX = m_sections.size() - 1.f;
        endSectionX = (endSectionX + 1.f < maxSectionX) ? (endSectionX + 1.f) : maxSectionX;

        float startSectionY = sectionY(rect.origin.y);
        startSectionY = (startSectionY - 1.f >= 0.f) ? (startSectionY - 1.f) : 0.f;

        float endSectionY = (float)(int)(sectionY(rect.origin.y + rect.size.height) + 1.f);

        int xStart = (int)startSectionX;
        int xEnd = (int)endSectionX;
        int yStart = (int)startSectionY;
        int yEnd = (int)endSectionY;

        for (int xi = xStart; xi <= xEnd; xi++) {
            auto* xBucket = m_sections[xi];
            if (!xBucket) continue;

            int bucketYMax = (int)xBucket->size() - 1;
            if (yEnd > bucketYMax) yEnd = bucketYMax;

            for (int yi = yStart; yi <= yEnd; yi++) {
                auto* yBucket = (*xBucket)[yi];
                if (!yBucket) continue;

                int count = (*m_sectionSizes[xi])[yi];

                for (int i = 0; i < count; i++) {
                    GameObject* object = (*yBucket)[i];
                    checkObject(object);
                }
            }
        }

        return objects;
    }
};

class $modify(GameObject) {
    $override
    void updateObjectEditorColor() {
        GameObject::updateObjectEditorColor();

        if (auto lel = LevelEditorLayer::get()) {
            if (lel->m_previewMode) return;
            m_updateEditorColor = true;
        }
    }
};

ccColor3B tintColorBoosted(const ccColor3B& original, const ccColor3B& tint, float ratio, float boost = 1.2f) {
    ccColor3B result = ie::blendColor(original, tint, ratio);

    float origV = std::max({original.r, original.g, original.b}) / 255.f;
    float resultV = std::max({result.r, result.g, result.b}) / 255.f;

    if (resultV < 1e-6f) return result;

    float mod = std::min(origV * boost, 1.f) / resultV;

    return {
        static_cast<GLubyte>(std::min(result.r * mod, 255.f)),
        static_cast<GLubyte>(std::min(result.g * mod, 255.f)),
        static_cast<GLubyte>(std::min(result.b * mod, 255.f))
    };
}

float greenRatio(const ccColor3B& color) {
    constexpr float min = 0.28f;
    constexpr float max = 0.62f;
    auto hsv = ie::rgbToHsv(color);

    float hueDist = std::min(
        std::fabs(hsv.h - 120.f),
        360.f - std::fabs(hsv.h - 120.f)
    ) / 180.f;

    float effectiveDist = 1.f - hsv.s * (1.f - hueDist);

    return min + effectiveDist * (max - min);
}

void setPreviewColor(GameObject* object, bool hovering) {
    if (object->m_isSelected) return;

    constexpr ccColor3B green = {0, 255, 0};
    constexpr ccColor3B greenDetail = {0, 204, 0};
    float ratio = hovering ? 0.5f : 1.f;

    float mainRatio = greenRatio(object->getColor()) * ratio;
    object->updateMainColor(tintColorBoosted(object->getColor(), green, mainRatio));

    if (auto spr = object->m_colorSprite) {
        float detailRatio = greenRatio(spr->getColor()) * ratio;
        object->updateSecondaryColor(tintColorBoosted(spr->getColor(), greenDetail, detailRatio));
    }

    if (hovering) return;

    object->CCSprite::setOpacity(std::clamp(object->getOpacity() + 100, 0, 255));
    if (auto spr = object->m_colorSprite) {
        spr->CCSprite::setOpacity(std::clamp(spr->getOpacity() + 80, 0, 255));
    }
}

bool isHoveringOverMenu(CCMenu* menu, const CCPoint& pos) {
    // https://github.com/altalk23/cocos2d-x-gd/blob/6bccfe7aecdbc32977395d50abcf385627b8f688/cocos2dx/menu_nodes/CCMenu.cpp#L226

    if (!menu->isEnabled() || !nodeIsVisible(menu)) return false;

    auto children = menu->getChildren();
    if (!children || children->count() == 0) return false;

    for (const auto& object : children->asExt()) {
        auto child = typeinfo_cast<CCMenuItem*>(object);
        if (!child || !child->isVisible() || !child->isEnabled()) continue;

        CCPoint local = child->convertToNodeSpace(pos);
        CCRect rect = child->rect();
        rect.origin = ccp(0, 0);

        if (rect.containsPoint(local)) return true;
    }

    return false;
}

bool isHoveringOverInput(CCTextInputNode* input, const CCPoint& pos) {
    if (!nodeIsVisible(input)) return false;

    auto parent = input->getParent();
    if (!parent) return false;

    CCPoint local = parent->convertToNodeSpace(pos);
    CCRect rect = CCRect(
        input->getPosition() - input->getScaledContentSize() / 2,
        input->getScaledContentSize()
    );

    return rect.containsPoint(local);
}

bool hoveringOverEditorUI(LevelEditorLayer* lel, const CCPoint& pos) {
    if (lel->m_editorUI->m_toolbarHeight >= pos.y) return false;

    auto handlers = CCTouchDispatcher::get()->m_pTargetedHandlers;

    for (const auto& handler : CCArrayExt<CCTargetedTouchHandler*>(handlers)) {
        CCTouchDelegate* delegate = handler->m_pDelegate;
        if (!delegate) continue;

        // theoretically there could be other types of delegates, but in practice
        // i think we only need to worry about menus and text inputs

        if (auto menu = typeinfo_cast<CCMenu*>(delegate)) {
            if (isHoveringOverMenu(menu, pos)) return false;
        } else if (auto input = typeinfo_cast<CCTextInputNode*>(delegate)) {
            if (isHoveringOverInput(input, pos)) return false;
        }
    }

    return true;
}

void ie::updateSelectPreview(LevelEditorLayer* lel, GameObject* object) {
    object->m_cycleIndex = 0;

    // lel->m_debugDrawNode->drawRect(
    //     lel->getObjectRect(object, false, false), {0, 0, 0, 0}, 0.5, {1, 0, 0, 1}, BorderAlignment::Center
    // );

    // SelectionBox box(lel, object, !eui->m_swipeActive);
    // box.draw(lel->m_debugDrawNode);
}

void ie::postUpdateSelectPreview(LevelEditorLayer* lel) {
    if (lel->m_playbackMode == PlaybackMode::Playing || !ie::isEditorTopLevel(lel)) return;

    EditorUI* eui = lel->m_editorUI;
    eui->m_cycledObjectIndex = 0;

    bool selecting = eui->m_swipeActive && ccpDistance(eui->m_swipeStart, eui->m_swipeEnd) > 2.f;
    CCArray* objects;

    if (selecting) {
        CCPoint start = lel->m_objectLayer->convertToNodeSpace(eui->m_swipeStart);
        CCPoint end = lel->m_objectLayer->convertToNodeSpace(eui->m_swipeEnd);

        float x = std::min(start.x, end.x);
        float y = std::min(start.y, end.y);
        float width = std::abs(start.x - end.x);
        float height = std::abs(start.y - end.y);

        objects = lel->objectsInRect({x, y, width, height}, false);
    } else {
#ifdef GEODE_IS_DESKTOP
        CCPoint mousePos = getMousePos();
        if (!hoveringOverEditorUI(lel, mousePos)) return;
        if (eui->m_snapObjectExists && eui->m_continueSwipe) return; // dragging an object with free move

        CCPoint pos = lel->m_objectLayer->convertToNodeSpace(mousePos);

        objects = CCArray::create();

        if (auto object = static_cast<SPLevelEditorLayer*>(lel)->betterObjectAtPosition(pos, true)) {
            objects->addObject(object);
        }
#else
        return;
#endif
    }

    std::unordered_set<GameObject*> previewed;

    for (const auto& object : CCArrayExt<GameObject*>(objects)) {
        if (previewed.contains(object)) continue;
        if (!eui->canSelectObject(object)) continue;

        setPreviewColor(object, !selecting);
        previewed.insert(object);

        if (object->m_linkedGroup == 0 || !ie::isLinkControlsEnabled(lel)) continue;
        CCArray* group = lel->getStickyGroup(object->m_linkedGroup);

        for (const auto& linked : CCArrayExt<GameObject*>(group)) {
            if (previewed.contains(linked)) continue;

            setPreviewColor(linked, !selecting);
            previewed.insert(linked);
        }
    }
}
