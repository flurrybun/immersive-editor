#include "ui/CheckboxPopup.hpp"
#include "util/ButtonHook.hpp"

#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(EditorPauseLayer) {
    $override
    bool init(LevelEditorLayer* layer) {
        if (!EditorPauseLayer::init(layer)) return false;

        if (Mod::get()->getSavedValue<bool>("object-rotation-drift-fix-shown")) {
            return true;
        }

        auto menu = getChildByID("resume-menu");

        std::array<ZStringView, 3> buttonIDs = {
            "save-and-play-button",
            "save-and-exit-button",
            "save-button"
        };

        for (const auto& id : buttonIDs) {
            auto button = static_cast<CCMenuItemSpriteExtra*>(menu->getChildByID(id));
            if (!button) continue;

            ie::hookButton(
                button,
                [this](CCObject* sender, ie::OriginalButtonCallback original) {
                    promptRotationDrift(std::move(original));
                }
            );
        }

        return true;
    }

    void promptRotationDrift(ie::OriginalButtonCallback original) {
        if (Mod::get()->getSavedValue<bool>("object-rotation-drift-fix-shown")) {
            std::move(original)();
            return;
        }

        CheckboxPopup::createAndShowOnce(
            "object-rotation-drift-fix",
            "The editor has a bug where objects with <cg>negative rotation values</c> repeatedly drift by <cy>+0.01</c> upon saving and exiting. How should <cj>Immersive Editor</c> handle this?",
            {
                { "fix-and-normalize", "Fix drift & normalize rotation", "Updates all rotation values to be positive, so if someone without this mod opens the level, existing rotated objects won't drift (recommended)." },
                { "fix-only", "Fix drift only", "Object rotation stays unchanged." },
                { "no-fix", "Don't fix" }
            },
            [this, original = std::move(original)](ZStringView id) {
                bool fixDrift = false;
                bool normalizeRotation = false;

                if (id == "fix-and-normalize") {
                    fixDrift = true;
                    normalizeRotation = true;
                } else if (id == "fix-only") {
                    fixDrift = true;
                }

                log::info("{}, fixDrift: {}, normalizeRotation: {}", id, fixDrift, normalizeRotation);

                Mod::get()->setSettingValue("fix-rotation-drift", fixDrift);
                Mod::get()->setSettingValue("normalize-rotation", normalizeRotation);

                std::move(original)();
            }
        );
    }
};
