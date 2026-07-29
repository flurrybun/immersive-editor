#include "events/PlaytestEvent.hpp"

#include <Geode/modify/LevelEditorLayer.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(LevelEditorLayer) {
    struct Fields {
        ListenerHandle playtestListener;
    };

    $override
    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        m_fields->playtestListener = PlaytestEvent().listen([this](PlaytestMode mode) {
            bool visible = !mode.isNot();

            resetPlayerParticles(m_player1, visible);
            resetPlayerParticles(m_player2, visible);
            resetParticle(m_glitterParticles, visible);
        });

        return true;
    }

    void resetPlayerParticles(PlayerObject* player, bool visible) {
        for (const auto& particle : CCArrayExt<CCParticleSystemQuad>(player->m_particleSystems)) {
            resetParticle(particle, visible);
        }
    }

    void resetParticle(CCParticleSystemQuad* particle, bool visible) {
        if (!particle || !particle->getParent()) return;

        if (!visible) {
            particle->stopSystem();
            particle->update(particle->getLife() + particle->getLifeVar() + 1.f);
        }

        particle->setVisible(visible);
    }
};
