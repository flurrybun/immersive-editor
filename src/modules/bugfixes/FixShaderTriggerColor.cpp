#include "core/SettingManager.hpp"

#include <Geode/modify/ShaderGameObject.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(ShaderGameObject) {
    $register_hooks("fix-shader-trigger-color");

    $override
    void customSetup() {
        // ⏺️ fix shader triggers using the object color for no reason

        ShaderGameObject::customSetup();
        setDefaultMainColorMode(0);
    }
};
