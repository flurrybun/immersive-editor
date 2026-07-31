#include "core/SettingManager.hpp"
#include "core/UpdateVisibility.hpp"
#include "util/ObjectIDs.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

$bind_setting(g_showColorblindIndicators, "show-colorblind-indicators");

bool isGuideEnabled() {
    // probably useless optimization for those (such as myself) who normally have these disabled

    return (
        !GameManager::get()->getGameVariable(GameVar::DisablePortalGuide) ||
        GameManager::get()->getGameVariable(GameVar::OrbGuide)
    );
}

void addGuideArt(LevelEditorLayer* lel, GameObject* object) {
    lel->addGuideArt(object);

    CCSprite* guideArt = static_cast<CCSprite*>(lel->m_indicatorSprites->lastObject());
    if (guideArt) guideArt->setUserFlag("is-indicator"_spr, true);
}

void removeGuideArt(LevelEditorLayer* lel, GameObject* object) {
    for (const auto& child : object->getChildrenExt()) {
        if (!child->getUserFlag("is-indicator"_spr)) continue;

        child->removeFromParent();
        lel->m_indicatorSprites->removeObject(child);
        object->m_hasCustomChild = false;
    }
}

$on_enable("show-colorblind-indicators") {
    if (!isGuideEnabled()) return;

    LevelEditorLayer* lel = ctx.m_lel;

    ctx.onObjectEvent([lel](GameObject* object, bool created) {
        if (!ie::object::isPortal(object) && !ie::object::isOrb(object)) return;

        if (created) {
            addGuideArt(lel, object);
        } else {
            removeGuideArt(lel, object);
        }
    });
}

$on_disable("show-colorblind-indicators") {
    if (!isGuideEnabled()) return;

    for (const auto& object : CCArrayExt<GameObject*>(ctx.m_lel->m_objects)) {
        if (!ie::object::isPortal(object) && !ie::object::isOrb(object)) continue;
        if (!object->m_hasCustomChild) continue;

        removeGuideArt(ctx.m_lel, object);
    }
}

void ie::updateColorblindIndicators(LevelEditorLayer* lel) {
    if (!g_showColorblindIndicators) return;

    lel->updateGuideArt();
}
