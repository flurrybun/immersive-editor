#include <Geode/modify/EnhancedGameObject.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include "misc/Utils.hpp"

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
            case 3027:

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

    $override
    void claimParticle() {
        // ⏺️ recolor particle on creation to match selected state

        GameObject::claimParticle();
        if (!shouldSelectParticle()) return;

        if (m_isSelected) {
            setParticleColor(getColor());
        } else {
            resetParticleColor();
        }
    }

    $override
    void selectObject(ccColor3B color) {
        // ⏺️ recolor particle on select

        GameObject::selectObject(color);
        if (!shouldSelectParticle()) return;

        setParticleColor(color);
    }

    $override
    void updateObjectEditorColor() {
        // ⏺️ recolor particle on deselect (GameObject::deselectObject is inlined)

        GameObject::updateObjectEditorColor();
        if (!shouldSelectParticle()) return;

        resetParticleColor();
    }

    $override
    void updateParticleOpacity(unsigned char opacity) {
        // ⏺️ lower particle opacity when on different editor layer

        if (!LevelEditorLayer::get() || !m_particle) {
            GameObject::updateParticleOpacity(opacity);
            return;
        }

        m_particle->setOpacity(ie::isObjectLayerVisible(this, LevelEditorLayer::get()) ? 255 : 50);
    }

    bool shouldSelectParticle() {
        return (
            m_editorEnabled &&
            m_particle &&
            !m_particleUseObjectColor &&
            m_objectID != 2065 // custom particle
        );
    }

    void setParticleColor(const ccColor3B& color) {
        ccColor4F color4F = to4F(to4B(color));
        setParticleColor(color4F, color4F, {}, {});
    }

    void setParticleColor(
        const ccColor4F& start, const ccColor4F& end, const ccColor4F& startVar, const ccColor4F& endVar
    ) {
        m_particle->setStartColor(start);
        m_particle->setEndColor(end);
        m_particle->setStartColorVar(startVar);
        m_particle->setEndColorVar(endVar);
    }

    void resetParticleColor() {
        switch (m_objectID) {
            case 36: // yellow orb
                return setParticleColor({1, 0.78431374, 0.19607843, 1}, {1, 0.39215687, 0.09803922, 1}, {0, 0, 0, 0.698125}, {});
            case 141: // pink orb
                return setParticleColor({1, 0.39215687, 1, 1}, {1, 0, 0.6862745, 1}, {0, 0, 0, 0.698125}, {});
            case 1333: // red orb
                return setParticleColor({1, 0.39215687, 0.09803922, 1}, {1, 0, 0, 1}, {0, 0, 0, 0.698125}, {});
            case 84: // blue orb
                return setParticleColor({0, 1, 1, 1}, {0, 0.49019608, 1, 1}, {0, 0, 0, 0.698125}, {});
            case 1022: // green orb
                return setParticleColor({0.29411766, 1, 0.29411766, 1}, {0, 1, 0, 1}, {0, 0, 0, 0.698125}, {});
            case 1330: // black orb
                return setParticleColor({1, 1, 1, 1}, {0, 0, 0, 1}, {0, 0, 0, 0.698125}, {});
            case 1704: // green dash orb
                return setParticleColor({0, 1, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 0.698125}, {});
            case 1751: // pink dash orb
                return setParticleColor({1, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 0.698125}, {});
            case 3004: // spider orb
                return setParticleColor({1, 0, 1, 1}, {0.39215687, 0.09803922, 1, 1}, {0, 0, 0, 0.698125}, {});
            case 35: // yellow pad
                return setParticleColor({1, 1, 0, 1}, {0, 0, 0, 1}, {}, {});
            case 140: // pink pad
                return setParticleColor({1, 0, 1, 1}, {1, 0, 1, 1}, {}, {});
            case 1332: // red pad
                return setParticleColor({1, 0.19607843, 0.19607843, 1}, {1, 0, 0, 1}, {}, {});
            case 67: // blue pad
                return setParticleColor({0, 1, 1, 1}, {0, 1, 1, 1}, {}, {});
            case 3005: // spider pad
                return setParticleColor({1, 0, 1, 1}, {0.39215687, 0.09803922, 1, 1}, {}, {});
            case 10: // blue portal
                return setParticleColor({0, 1, 1, 0.5}, {0, 1, 1, 1}, {}, {0, 0.5, 0, 0});
            case 11: // yellow portal
                return setParticleColor({1, 1, 0, 0.5}, {1, 1, 0, 1}, {}, {});
            case 2926: // green portal
                return setParticleColor({0, 1, 0, 1}, {0, 1, 0, 1}, {}, {});
            case 12: // cube portal
                return setParticleColor({0.3, 1, 0, 0.5}, {0.3, 1, 0, 1}, {0.3, 0, 0, 0}, {0.3, 0, 0, 0});
            case 13: // ship portal
                return setParticleColor({1, 0, 1, 0.5}, {1, 0, 1, 1}, {0, 0.3, 0, 0}, {0, 0.3, 0, 0});
            case 47: // ball portal
                return setParticleColor({1, 0.39215687, 0, 1}, {1, 0.39215687, 0, 1}, {}, {});
            case 111: // ufo portal
                return setParticleColor({1, 0.78431374, 0, 1}, {1, 0.39215687, 0, 1}, {}, {});
            case 660: // wave portal
                return setParticleColor({0, 0.78431374, 1, 1}, {0, 0.39215687, 1, 1}, {}, {});
            case 745: // robot portal
                return setParticleColor({0.5882353, 0.5882353, 0.5882353, 1}, {0.19607843, 0.19607843, 0.29411766, 1}, {}, {});
            case 45: // orange mirror portal
                return setParticleColor({1, 0.5882353, 0, 1}, {1, 0.5882353, 0, 1}, {}, {});
            case 46: // blue mirror portal
                return setParticleColor({0, 1, 1, 0.5}, {0, 1, 1, 1}, {}, {0, 0.5, 0, 0});
            case 99: // green size portal
                return setParticleColor({0, 1, 0, 1}, {0, 1, 0, 1}, {0.25, 0, 0.25, 0.5}, {});
            case 101: // pink size portal
                return setParticleColor({1, 0, 1, 1}, {1, 0, 1, 1}, {0, 0.5, 0, 0.5}, {});
            case 286: // orange dual portal
                return setParticleColor({1, 0.78431374, 0, 1}, {1, 0.39215687, 0, 1}, {0, 0.5, 0, 0.5}, {});
            case 287: // blue dual portal
                return setParticleColor({0, 0.78431374, 1, 1}, {0, 0.39215687, 1, 1}, {0, 0.5, 0, 0.5}, {});
            case 747: // linked blue teleport
            case 2902: // blue teleport portal
                return setParticleColor({0, 1, 1, 1}, {0, 0.39215687, 0.5882353, 1}, {}, {});
            case 749: // linked orange teleport
            case 2064: // orange teleport portal
                return setParticleColor({1, 0.78431374, 0, 1}, {1, 0.39215687, 0, 1}, {}, {});
            case 1331: // spider portal
                return setParticleColor({0.78431374, 0, 1, 1}, {0.78431374, 0, 1, 1}, {}, {});
            case 1933: // swing portal
                return setParticleColor({1, 1, 0, 1}, {1, 0.78431374, 0, 1}, {}, {});
            case 200: // slow speed portal
                return setParticleColor({1, 0.9, 0, 0.8}, {1, 0.9, 0, 0}, {0, 0, 0, 0.2}, {});
            case 201: // normal speed portal
                return setParticleColor({0, 0.8, 1, 0.8}, {0, 0.8, 1, 0}, {0, 0, 0, 0.2}, {});
            case 202: // fast speed portal
                return setParticleColor({0, 1, 0.2, 0.8}, {0, 1, 0.2, 0}, {0, 0, 0, 0.2}, {});
            case 203: // faster speed portal
                return setParticleColor({1, 0.6, 1, 0.8}, {1, 0.3, 1, 0}, {0, 0, 0, 0.2}, {});
            case 1334: // fastest speed portal
                return setParticleColor({1, 0, 0, 0.8}, {1, 0, 0, 0}, {0, 0, 0, 0.2}, {});
        }
    }
};

class $modify(LevelEditorLayer) {
    $override
    void updateVisibility(float dt) {
        // ⏺️ update particle visibility based on 'no particles' setting

        LevelEditorLayer::updateVisibility(dt);

        for (const auto& object : m_activeObjects) {
            if (!object->m_particle || object->m_objectID == 2065) continue;

            object->m_particle->setVisible(!object->m_hasNoParticles);
        }
    }
};
