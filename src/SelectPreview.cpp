#include "UpdateVisibility.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

ccColor3B tintColor(const ccColor3B& original, const ccColor3B& tint, float ratio) {
    return {
        static_cast<GLubyte>(original.r * (1 - ratio) + tint.r * ratio),
        static_cast<GLubyte>(original.g * (1 - ratio) + tint.g * ratio),
        static_cast<GLubyte>(original.b * (1 - ratio) + tint.b * ratio)
    };
}

void setPreviewColor(GameObject* object) {
    if (object->m_isSelected) return;
    constexpr float ratio = 0.3f;

    object->updateMainColor(tintColor(object->getColor(), {0, 255, 0}, ratio));
    object->CCSprite::setOpacity(std::clamp(object->getOpacity() + 120, 0, 255));

    if (auto spr = object->m_colorSprite) {
        object->updateSecondaryColor(tintColor(spr->getColor(), {0, 204, 0}, ratio));
        spr->setOpacity(std::clamp(spr->getOpacity() * (1.f + ratio), 0.f, 255.f));
    }
}

void ie::updateSelectPreview(LevelEditorLayer* lel) {
    EditorUI* eui = lel->m_editorUI;
    if (!eui->m_swipeActive) return;

    CCPoint start = lel->m_objectLayer->convertToNodeSpace(eui->m_swipeStart);
    CCPoint end = lel->m_objectLayer->convertToNodeSpace(eui->m_swipeEnd);

    if (start == end) return;

    float x = std::min(start.x, end.x);
    float y = std::min(start.y, end.y);
    float width = std::abs(start.x - end.x);
    float height = std::abs(start.y - end.y);

    CCArray* objects = lel->objectsInRect({x, y, width, height}, false);

    std::unordered_set<GameObject*> previewed;

    for (const auto& object : CCArrayExt<GameObject*>(objects)) {
        if (previewed.contains(object)) continue;

        setPreviewColor(object);
        previewed.insert(object);

        if (object->m_linkedGroup == 0) continue;
        CCArray* group = lel->getStickyGroup(object->m_linkedGroup);

        for (const auto& linked : CCArrayExt<GameObject*>(group)) {
            if (previewed.contains(linked)) continue;

            setPreviewColor(linked);
            previewed.insert(linked);
        }
    }
}
