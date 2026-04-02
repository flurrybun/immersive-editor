#pragma once

#include <Geode/binding/LevelEditorLayer.hpp>

using namespace geode::prelude;

namespace ie {
    GameObject* objectAtPosition(LevelEditorLayer* lel, const CCPoint& position, bool hovering);
    std::vector<GameObject*> objectsAtPosition(LevelEditorLayer* lel, const CCPoint& position, bool hovering);
}
