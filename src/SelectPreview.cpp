#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/GameObject.hpp>
#include "UpdateVisibility.hpp"
#include "misc/Utils.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(SPLevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        std::unordered_set<WeakRef<GameObject>> prevHoveredObjects;
        std::list<WeakRef<GameObject>> cycledObjects;

        bool dontUpdateHistory = false;
        bool dontIgnoreSelected = false;
    };

    $override
    CCArray* objectsAtPosition(CCPoint position) {
        // toVector may be a bit inefficient but there shouldn't be any more than a dozen objects or so
        // and it lets us use nice functions instead of manual for loop bs

        std::vector<GameObject*> objects = LevelEditorLayer::objectsAtPosition(position)
            ->asExt<GameObject*>().toVector();

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

        if (!m_fields->dontUpdateHistory) {
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

        // ignore unselectable objects

        if (m_fields->dontIgnoreSelected) {
            std::erase_if(objects, [&](GameObject* object) {
                return !m_editorUI->canSelectObject(object);
            });
        } else {
            std::erase_if(objects, [&](GameObject* object) {
                return object->m_isSelected || !m_editorUI->canSelectObject(object);
            });
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

        // find the first object that isn't already cycled through

        auto object = ranges::find(objects, [&](GameObject* obj) {
            return !ranges::contains(m_fields->cycledObjects, obj);
        });

        if (!object || !*object) return CCArray::create();

        if (!m_fields->dontUpdateHistory) m_fields->cycledObjects.push_front(*object);
        return CCArray::createWithObject(*object);
    }
};

class $modify(EditorUI) {
    $override
    bool ccTouchBegan(CCTouch* touch, CCEvent* event) {
        static_cast<SPLevelEditorLayer*>(m_editorLayer)->m_fields->dontIgnoreSelected = true;
        auto ret = EditorUI::ccTouchBegan(touch, event);
        static_cast<SPLevelEditorLayer*>(m_editorLayer)->m_fields->dontIgnoreSelected = false;

        return ret;
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

    if (!menu->m_bVisible || !menu->m_bEnabled) return false;

    for (CCNode* c = menu->m_pParent; c != nullptr; c = c->getParent()) {
        if (!c->isVisible()) return false;
    }

    if (!menu->m_pChildren || menu->m_pChildren->count() == 0) return false;

    for (const auto& object : menu->m_pChildren->asExt()) {
        auto child = typeinfo_cast<CCMenuItem*>(object);
        if (!child || !child->isVisible() || !child->isEnabled()) continue;

        CCPoint local = child->convertToNodeSpace(pos);
        CCRect rect = child->rect();
        rect.origin = ccp(0, 0);

        if (rect.containsPoint(local)) return true;
    }

    return false;
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
        }
        // } else if (auto input = typeinfo_cast<CCTextInputNode*>(delegate)) {
        //     if (input->isVisible() && input->getBoundingBox().containsPoint(pos)) return false;
        // }
    }

    return true;
}

void ie::updateSelectPreview(LevelEditorLayer* lel, GameObject* object) {
    object->m_unk460 = 0;
}

void ie::postUpdateSelectPreview(LevelEditorLayer* lel) {
    if (lel->m_playbackMode == PlaybackMode::Playing || !ie::isEditorTopLevel(lel)) return;

    EditorUI* eui = lel->m_editorUI;
    eui->m_cycledObjectIndex = 0;

    CCArray* objects;
    bool hovering = false;

    CCPoint start = lel->m_objectLayer->convertToNodeSpace(eui->m_swipeStart);
    CCPoint end = lel->m_objectLayer->convertToNodeSpace(eui->m_swipeEnd);

    if (eui->m_swipeActive && start != end) {
        float x = std::min(start.x, end.x);
        float y = std::min(start.y, end.y);
        float width = std::abs(start.x - end.x);
        float height = std::abs(start.y - end.y);

        objects = lel->objectsInRect({x, y, width, height}, false);
    } else {
#ifdef GEODE_IS_DESKTOP
        CCPoint mousePos = getMousePos();
        if (!hoveringOverEditorUI(lel, mousePos)) return;

        CCPoint pos = lel->m_objectLayer->convertToNodeSpace(mousePos);

        objects = CCArray::create();
        hovering = true;

        static_cast<SPLevelEditorLayer*>(lel)->m_fields->dontUpdateHistory = true;
        auto hoveredObjects = static_cast<SPLevelEditorLayer*>(lel)->objectsAtPosition(pos);
        static_cast<SPLevelEditorLayer*>(lel)->m_fields->dontUpdateHistory = false;

        if (auto object = static_cast<GameObject*>(hoveredObjects->firstObject())) {
            objects->addObject(object);
        }
#endif
    }

    std::unordered_set<GameObject*> previewed;

    for (const auto& object : CCArrayExt<GameObject*>(objects)) {
        if (previewed.contains(object)) continue;
        if (!eui->canSelectObject(object)) continue;

        setPreviewColor(object, hovering);
        previewed.insert(object);

        if (object->m_linkedGroup == 0 || !ie::isLinkControlsEnabled(lel)) continue;
        CCArray* group = lel->getStickyGroup(object->m_linkedGroup);

        for (const auto& linked : CCArrayExt<GameObject*>(group)) {
            if (previewed.contains(linked)) continue;

            setPreviewColor(linked, hovering);
            previewed.insert(linked);
        }
    }
}
