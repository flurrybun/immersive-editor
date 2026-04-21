#include "core/SettingManager.hpp"
#include "util/Editor.hpp"

#include <alphalaneous.good_grid/include/DrawGridAPI.hpp>
#include <alphalaneous.good_grid/include/DrawLayers.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>

using namespace geode::prelude;

$bind_setting(g_betterDurationLines, "better-duration-lines");

$on_enable("better-duration-lines") {
    // ⏺️ lower duration line opacity when on a different layer
    LevelEditorLayer* lel = ctx.m_lel;

    DrawGridAPI::get().getNode<DurationLines>("duration-lines").inspect([&](DurationLines& lines) {
        lines.setPropertiesForObject([&](LineColor& color, EffectGameObject* object, float& lineWidth) {
            if (!g_betterDurationLines || !object || !lel) return;

            GLubyte lineOpacity = ie::isObjectLayerVisible(object, lel) ? 115 : 23;
            color = LineColor(255, 255, 255, lineOpacity);
        });
    });
}
