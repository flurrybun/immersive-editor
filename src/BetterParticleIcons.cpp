#include "UpdateVisibility.hpp"
#include "misc/Utils.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

void ie::updateParticleIcon(LevelEditorLayer* lel, GameObject* object) {
    // ⏺️ make 2.1 particle icons low opacity, like 2.2 particles
    // ⏺️ lower particle icon opacity when on a different layer

    unsigned char opacity = 0;

    if (lel->m_playbackMode != PlaybackMode::Playing && !lel->m_hideParticleIcons) {
        opacity = ie::isObjectLayerVisible(object, lel) ? 50 : 8;
    }

    if (object->m_objectID == 1586 || object->m_objectID == 1700) {
        CCParticleSystemQuad* prevParticle = object->m_particle;
        object->m_particle = nullptr;

        object->CCSprite::setOpacity(opacity);
        if (object->m_colorSprite) object->m_colorSprite->setOpacity(opacity);

        object->m_particle = prevParticle;
    } else if (object->m_objectID == 2065) {
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
