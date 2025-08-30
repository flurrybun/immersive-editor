#include <Geode/modify/EnhancedGameObject.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(EnhancedGameObject) {
    $override
    void customSetup() {
        // ⏺️ particles for orbs, pads, portals, and the 2.1 fireball

        if (!LevelEditorLayer::get() || !hasParticles()) {
            EnhancedGameObject::customSetup();
            return;
        }

        bool prevHNP = m_hasNoParticles;
        m_hasNoParticles = false;
        m_editorEnabled = false;

        EnhancedGameObject::customSetup();

        m_hasNoParticles = prevHNP;
        m_editorEnabled = true;
    }

    bool hasParticles() {
        switch (m_objectID) {
            // orbs & pads

            case 36:
            case 35:
            case 84:
            case 67:
            case 141:
            case 140:
            case 1333:
            case 1332:
            case 1330:
            case 1594:
            case 1022:
            case 1704:
            case 1751:
            case 3004:
            case 3005:

            // gamemode portals

            case 10:
            case 11:
            case 12:
            case 13:
            case 45:
            case 46:
            case 47:
            case 99:
            case 101:
            case 111:
            case 286:
            case 287:
            case 660:
            case 745:
            case 747:
            case 2902:
            case 749:
            case 2064:
            case 1331:
            case 1933:
            case 2926:

            // speed portals

            case 200:
            case 201:
            case 202:
            case 203:
            case 1334:

            // fireball

            case 1583:

                return true;
            default:
                return false;
        }
    }
};

class $modify(GameObject) {
    $override
    void customSetup() {
        // ⏺️ particles for 2.1 particle objects

        bool is21Particle = m_objectID == 1586 || m_objectID == 1700;

        if (!LevelEditorLayer::get() || !is21Particle) {
            GameObject::customSetup();
            return;
        }

        bool prevHNP = m_hasNoParticles;
        m_hasNoParticles = false;
        m_editorEnabled = false;

        GameObject::customSetup();

        m_hasNoParticles = prevHNP;
        m_editorEnabled = true;
    }

    $override
    void commonInteractiveSetup() {
        // ⏺️ particles for interactive objects like keys

        if (!LevelEditorLayer::get()) {
            GameObject::commonInteractiveSetup();
            return;
        }

        m_editorEnabled = false;
        GameObject::commonInteractiveSetup();
        m_editorEnabled = true;
    }

    $override
    CCParticleSystemQuad* createAndAddParticle(int p0, char const* p1, int p2, tCCPositionType p3) {
        // ⏺️ show particles in editor

        if (!LevelEditorLayer::get()) {
            return GameObject::createAndAddParticle(p0, p1, p2, p3);
        }

        GameManager::get()->m_playLayer = reinterpret_cast<PlayLayer*>(GJBaseGameLayer::get());

        CCParticleSystemQuad* ret = GameObject::createAndAddParticle(p0, p1, p2, p3);

        GameManager::get()->m_playLayer = nullptr;
        return ret;
    }
};

class $modify(LevelEditorLayer) {
    $override
    void updateVisibility(float dt) {
        LevelEditorLayer::updateVisibility(dt);

        for (const auto& object : m_activeObjects) {
            if (!object->m_particle || object->m_objectID == 2065) continue;

            object->m_particle->setVisible(!object->m_hasNoParticles);
        }
    }
};
