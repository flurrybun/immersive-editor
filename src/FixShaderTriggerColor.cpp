#include <Geode/modify/ShaderGameObject.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(ShaderGameObject) {
    $override
    void customSetup() {
        // ⏺️ fix shader triggers using the object color for no reason

        ShaderGameObject::customSetup();
        setDefaultMainColorMode(0);
    }
};
