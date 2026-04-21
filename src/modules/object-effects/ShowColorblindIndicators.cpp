#include "core/SettingManager.hpp"
#include "events/ObjectEvent.hpp"
#include "util/ObjectIDs.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

$on_enable("show-colorblind-indicators") {
    ctx.addEventListener(ObjectEvent(), [lel = ctx.m_lel](GameObject* object, bool created) {
        if (created) lel->addGuideArt(object);
    });
}

$on_disable("show-colorblind-indicators") {
    for (const auto& object : CCArrayExt<GameObject*>(ctx.m_lel->m_objects)) {
        if (!ie::object::isPortal(object) && !ie::object::isOrb(object)) continue;
        if (!object->m_hasCustomChild) continue;

        CCNode* child = object->getChildByTag(100);
        if (!child) child = object->getChildByTag(101);

        if (!child) continue;

        child->removeFromParent();
        object->m_hasCustomChild = false;
    }
}
