#pragma once

#include <Geode/binding/LevelEditorLayer.hpp>

namespace ie {
    bool isAmazon();
    bool inEditor();

    bool isEditorTopLevel(LevelEditorLayer* lel);
    bool isObjectLayerVisible(GameObject* object, LevelEditorLayer* lel);
    bool isLinkControlsEnabled(LevelEditorLayer* lel);
}
