#include <Geode/modify/LevelEditorLayer.hpp>
#include "misc/Utils.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(LevelEditorLayer) {
    $override
    void updateVisibility(float dt) {
        // ⏺️ make 2.1 particle icons low opacity, like 2.2 particles
        // ⏺️ lower particle icon opacity when on a different layer

        LevelEditorLayer::updateVisibility(dt);

        for (size_t i = 0; i < m_activeObjectsCount; i++) {
            GameObject* object = m_activeObjects[i];
            unsigned char opacity = 0;

            if (m_playbackMode != PlaybackMode::Playing && !m_hideParticleIcons) {
                opacity = ie::isObjectLayerVisible(object, this) ? 50 : 8;
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
    }
};
