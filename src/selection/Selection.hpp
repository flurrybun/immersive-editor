#pragma once

#define GEODE_DEFINE_EVENT_EXPORTS

#include <Selection.hpp>

#undef GEODE_EVENT_EXPORT_NORES
#define GEODE_EVENT_EXPORT_NORES(...)
#undef MY_MOD_ID
#undef GEODE_DEFINE_EVENT_EXPORTS

namespace ie {
    std::vector<GameObject*> objectsAtPosition(LevelEditorLayer* lel, const cocos2d::CCPoint& position, bool selecting);
    GameObject* objectAtPosition(LevelEditorLayer* lel, const cocos2d::CCPoint& position, bool selecting);
}
