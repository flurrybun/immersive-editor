#include "core/SettingManager.hpp"
#include "util/ObjectIDs.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

$on_enable("show-colorblind-indicators") {
    LevelEditorLayer* lel = ctx.m_lel;

    ctx.onObjectEvent([lel](GameObject* object, bool created) {
        if (created) lel->addGuideArt(object);
    });
}

$on_disable("show-colorblind-indicators") {
    for (const auto& object : CCArrayExt<GameObject*>(ctx.m_lel->m_objects)) {
        if (!ie::object::isPortal(object) && !ie::object::isOrb(object)) continue;
        if (!object->m_hasCustomChild) continue;

        for (const auto& child : object->getChildrenExt()) {
            if (child->getZOrder() != 100 && child->getZOrder() != 101) continue;

            child->removeFromParent();
            object->m_hasCustomChild = false;
        }
    }
}
