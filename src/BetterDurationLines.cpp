#include <Geode/modify/LevelEditorLayer.hpp>
#include <alphalaneous.good_grid/include/DrawGridAPI.hpp>
#include <alphalaneous.good_grid/include/DrawLayers.hpp>
#include "misc/Utils.hpp"

using namespace geode::prelude;

class $modify(LevelEditorLayer) {
    $override
    bool init(GJGameLevel* p0, bool p1) {
        // ⏺️ lower duration line opacity when on a different layer

        if (!LevelEditorLayer::init(p0, p1)) return false;

        DrawGridAPI::get().getNode<DurationLines>("duration-lines").inspect([&](DurationLines& lines) {
            lines.setPropertiesForObject([&](LineColor& color, EffectGameObject* object, float& lineWidth) {
                if (this != LevelEditorLayer::get()) return;

                GLubyte lineOpacity = ie::isObjectLayerVisible(object, this) ? 115 : 23;
                color = LineColor(255, 255, 255, lineOpacity);
            });
        });

        return true;
    }
};
