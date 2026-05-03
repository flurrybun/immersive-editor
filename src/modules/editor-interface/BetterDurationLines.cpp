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

    DrawGridAPI::get().getNode<DurationLines>("duration-lines").inspect([lel](DurationLines& lines) {
        lines.setPropertiesForObject([lel](LineColor& color, EffectGameObject* object, float& lineWidth) {
            if (!g_betterDurationLines || !object) return;

            // sometimes the editor is invalid and i don't really know why
            if (lel != LevelEditorLayer::get()) return;

            GLubyte lineOpacity = ie::isObjectLayerVisible(object, lel) ? 115 : 23;
            color = LineColor(255, 255, 255, lineOpacity);
        });
    });
}
