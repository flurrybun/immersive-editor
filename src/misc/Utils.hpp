#pragma once

#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/cocos/include/ccTypes.h>

using namespace geode::prelude;

namespace ie {
    bool isAmazon();

    bool isEditorTopLevel(LevelEditorLayer* lel);
    bool isObjectLayerVisible(GameObject* object, LevelEditorLayer* lel);
    bool isLinkControlsEnabled(LevelEditorLayer* lel);

    struct HSV { float h, s, v; };

    HSV rgbToHsv(const ccColor3B& color);
    ccColor3B blendColor(const ccColor3B& first, const ccColor3B& second, float ratio);
}
