#include <Geode/modify/ShaderGameObject.hpp>
#include "UpdateVisibility.hpp"
#include "misc/Utils.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

void ie::updateDetailColorOpacity(LevelEditorLayer* lel, GameObject* object) {
    // ⏺️ fix detail color not lowering opacity when viewing a different editor layer & preview mode is disabled

    if (lel->m_previewMode || ie::isObjectLayerVisible(object, lel)) return;

    auto colorSpr = object->m_colorSprite;
    if (!colorSpr) return;

    colorSpr->setOpacity(colorSpr->getOpacity() * (50.f / 255.f));

    for (const auto& childSpr : colorSpr->getChildrenExt<CCSprite>()) {
        childSpr->setOpacity(childSpr->getOpacity() * (50.f / 255.f));
    }
}
