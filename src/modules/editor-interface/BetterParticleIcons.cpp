#include "core/SettingManager.hpp"
#include "core/UpdateVisibility.hpp"
#include "util/Editor.hpp"
#include "util/Temporary.hpp"
#include "util/ObjectIDs.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

$bind_setting(g_betterParticleIcons, "better-particle-icons");

void ie::updateParticleIcon(LevelEditorLayer* lel, GameObject* object) {
    if (!g_betterParticleIcons) return;

    // ⏺️ make 2.1 particle icons low opacity, like 2.2 particles
    // ⏺️ lower particle icon opacity when on a different layer

    unsigned char opacity = 0;

    if (lel->m_playbackMode != PlaybackMode::Playing && !lel->m_hideParticleIcons) {
        opacity = ie::isObjectLayerVisible(object, lel) ? 50 : 8;
    }

    if (ie::object::is21Particle(object)) {
        ie::withTemporary({
            { &object->m_particle, nullptr }
        }, [&] {
            object->CCSprite::setOpacity(opacity);
            if (object->m_colorSprite) object->m_colorSprite->setOpacity(opacity);
        });
    } else if (ie::object::isCustomParticle(object)) {
        if (auto sprite = object->getChildByType<CCSprite>(0)) {
            if (auto subSprite = sprite->getChildByType<CCSprite>(0)) {
                subSprite->setOpacity(opacity);
            }
        }

        if (auto sprite = object->getChildByType<CCSprite>(1)) {
            sprite->setOpacity(opacity);
        }
    }
}
