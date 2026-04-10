#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include "../UpdateVisibility.hpp"
#include "../misc/Utils.hpp"
#include "Selection.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

// i moved some functions to other files so now this hook just looks silly :(

class $modify(SPLevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        std::unordered_set<WeakRef<GameObject>> prevHoveredObjects;
    };
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

    float origV = std::max({ original.r, original.g, original.b }) / 255.f;
    float resultV = std::max({ result.r, result.g, result.b }) / 255.f;

    if (resultV < 1e-6f) return result;

    float mod = std::min(origV * boost, 1.f) / resultV;

    return {
        static_cast<GLubyte>(std::min(result.r * mod, 255.f)),
        static_cast<GLubyte>(std::min(result.g * mod, 255.f)),
        static_cast<GLubyte>(std::min(result.b * mod, 255.f))
    };
}

float hueRatio(const ccColor3B& color, float targetHue) {
    constexpr float min = 0.28f;
    constexpr float max = 0.62f;
    auto hsv = ie::rgbToHsv(color);

    float hueDist = std::min(
        std::fabs(hsv.h - targetHue),
        360.f - std::fabs(hsv.h - targetHue)
    ) / 180.f;

    float effectiveDist = 1.f - hsv.s * (1.f - hueDist);

    return min + effectiveDist * (max - min);
}

void ie::setPreviewColor(GameObject* object, const ccColor3B& color, bool selecting) {
    if (object->m_isSelected) return;

    float hue = ie::rgbToHsv(color).h;
    float ratio = selecting ? 1.f : 0.5f;

    float mainRatio = hueRatio(object->getColor(), hue) * ratio;
    object->updateMainColor(tintColorBoosted(object->getColor(), color, mainRatio));

    if (auto spr = object->m_colorSprite) {
        float detailRatio = hueRatio(spr->getColor(), hue) * ratio;
        ccColor3B detailColor = {
            static_cast<GLubyte>(color.r * 0.8f),
            static_cast<GLubyte>(color.g * 0.8f),
            static_cast<GLubyte>(color.b * 0.8f)
        };

        object->updateSecondaryColor(tintColorBoosted(spr->getColor(), detailColor, detailRatio));
    }

    if (!selecting) return;

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

    // EditorUI* eui = lel->m_editorUI;
    // bool selecting = eui->m_swipeActive && ccpDistance(eui->m_swipeStart, eui->m_swipeEnd) > 2.f;

    // SelectionBox box(lel, object, !selecting);
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

        if (auto object = ie::objectAtPosition(lel, pos, false)) {
            objects->addObject(object);
        }
#else
        return;
#endif
    }

    constexpr ccColor3B selectColor = { 0, 255, 0 };
    std::unordered_set<GameObject*> previewed;

    for (const auto& object : CCArrayExt<GameObject*>(objects)) {
        if (previewed.contains(object)) continue;
        if (!eui->canSelectObject(object)) continue;

        setPreviewColor(object, selectColor, !selecting);
        previewed.insert(object);

        if (object->m_linkedGroup == 0 || !ie::isLinkControlsEnabled(lel)) continue;
        CCArray* group = lel->getStickyGroup(object->m_linkedGroup);

        for (const auto& linked : CCArrayExt<GameObject*>(group)) {
            if (previewed.contains(linked)) continue;

            setPreviewColor(linked, selectColor, !selecting);
            previewed.insert(linked);
        }
    }
}
