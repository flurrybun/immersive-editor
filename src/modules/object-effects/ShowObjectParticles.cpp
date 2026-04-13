#include "core/SettingManager.hpp"
#include "core/UpdateVisibility.hpp"
#include "util/Editor.hpp"
#include "util/Temporary.hpp"
#include "util/ObjectIDs.hpp"

#include <Geode/modify/EnhancedGameObject.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

$bind_setting(g_showObjectParticles, "show-object-particles");
bool s_dontCreateParticles = false;

class $modify(EnhancedGameObject) {
    $register_hooks("show-object-particles");

    $override
    void customSetup() {
        // ⏺️ particles for orbs, pads, portals, and the 2.1 fireball

        if (!ie::inEditor() || !hasParticles()) {
            EnhancedGameObject::customSetup();
            return;
        }

        ie::withTemporary({
            { &m_hasNoParticles, false },
            { &m_editorEnabled, false },
        }, [&] {
            EnhancedGameObject::customSetup();
        });
    }

    bool hasParticles() {
        return (
            ie::object::isOrb(this) ||
            ie::object::isPad(this) ||
            ie::object::isPortal(this) ||
            ie::object::isSpeedPortal(this) ||
            // ie::object::is21Particle(this) ||
            ie::object::isMovingFireball(this)
        );
    }
};

class $modify(GameObject) {
    $register_hooks("show-object-particles");

    $override
    void customSetup() {
        // ⏺️ particles for 2.1 particle objects

        if (!ie::inEditor() || !ie::object::is21Particle(this)) {
            GameObject::customSetup();
            return;
        }

        ie::withTemporary({
            { &m_hasNoParticles, false },
            { &m_editorEnabled, false },
        }, [&] {
            GameObject::customSetup();
        });
    }

    $override
    void commonInteractiveSetup() {
        // ⏺️ particles for interactive objects like keys

        if (!ie::inEditor()) {
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

        if (!ie::inEditor()) {
            return GameObject::createAndAddParticle(p0, p1, p2, p3);
        }

        if (s_dontCreateParticles) return nullptr;

        CCParticleSystemQuad* ret = nullptr;

        ie::withFakePlayLayer([&] {
            ret = GameObject::createAndAddParticle(p0, p1, p2, p3);
        });

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

        if (!ie::inEditor() || !m_particle) {
            GameObject::updateParticleOpacity(opacity);
            return;
        }

        bool isObjectLayerVisible = ie::isObjectLayerVisible(this, LevelEditorLayer::get());
        float mod = isObjectLayerVisible ? 1.0f : 5.1f;

        GameObject::updateParticleOpacity(opacity * mod);
        m_particle->setOpacity(isObjectLayerVisible ? 255 : 50);
    }

    bool shouldSelectParticle() {
        return (
            m_editorEnabled &&
            m_particle &&
            !m_particleUseObjectColor &&
            !ie::object::isCustomParticle(this)
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
            case ie::object::YellowOrb:
                return setParticleColor({1, 0.78431374, 0.19607843, 1}, {1, 0.39215687, 0.09803922, 1}, {0, 0, 0, 0.698125}, {});
            case ie::object::PinkOrb:
                return setParticleColor({1, 0.39215687, 1, 1}, {1, 0, 0.6862745, 1}, {0, 0, 0, 0.698125}, {});
            case ie::object::RedOrb:
                return setParticleColor({1, 0.39215687, 0.09803922, 1}, {1, 0, 0, 1}, {0, 0, 0, 0.698125}, {});
            case ie::object::BlueOrb:
                return setParticleColor({0, 1, 1, 1}, {0, 0.49019608, 1, 1}, {0, 0, 0, 0.698125}, {});
            case ie::object::GreenOrb:
                return setParticleColor({0.29411766, 1, 0.29411766, 1}, {0, 1, 0, 1}, {0, 0, 0, 0.698125}, {});
            case ie::object::BlackOrb:
                return setParticleColor({1, 1, 1, 1}, {0, 0, 0, 1}, {0, 0, 0, 0.698125}, {});
            case ie::object::GreenDashOrb:
                return setParticleColor({0, 1, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 0.698125}, {});
            case ie::object::PinkDashOrb:
                return setParticleColor({1, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 0.698125}, {});
            case ie::object::SpiderOrb:
                return setParticleColor({1, 0, 1, 1}, {0.39215687, 0.09803922, 1, 1}, {0, 0, 0, 0.698125}, {});
            case ie::object::YellowPad:
                return setParticleColor({1, 1, 0, 1}, {0, 0, 0, 1}, {}, {});
            case ie::object::PinkPad:
                return setParticleColor({1, 0, 1, 1}, {1, 0, 1, 1}, {}, {});
            case ie::object::RedPad:
                return setParticleColor({1, 0.19607843, 0.19607843, 1}, {1, 0, 0, 1}, {}, {});
            case ie::object::BluePad:
                return setParticleColor({0, 1, 1, 1}, {0, 1, 1, 1}, {}, {});
            case ie::object::SpiderPad:
                return setParticleColor({1, 0, 1, 1}, {0.39215687, 0.09803922, 1, 1}, {}, {});
            case ie::object::BlueGravityPortal:
                return setParticleColor({0, 1, 1, 0.5}, {0, 1, 1, 1}, {}, {0, 0.5, 0, 0});
            case ie::object::YellowGravityPortal:
                return setParticleColor({1, 1, 0, 0.5}, {1, 1, 0, 1}, {}, {});
            case ie::object::GreenGravityPortal:
                return setParticleColor({0, 1, 0, 1}, {0, 1, 0, 1}, {}, {});
            case ie::object::CubePortal:
                return setParticleColor({0.3, 1, 0, 0.5}, {0.3, 1, 0, 1}, {0.3, 0, 0, 0}, {0.3, 0, 0, 0});
            case ie::object::ShipPortal:
                return setParticleColor({1, 0, 1, 0.5}, {1, 0, 1, 1}, {0, 0.3, 0, 0}, {0, 0.3, 0, 0});
            case ie::object::BallPortal:
                return setParticleColor({1, 0.39215687, 0, 1}, {1, 0.39215687, 0, 1}, {}, {});
            case ie::object::UfoPortal:
                return setParticleColor({1, 0.78431374, 0, 1}, {1, 0.39215687, 0, 1}, {}, {});
            case ie::object::WavePortal:
                return setParticleColor({0, 0.78431374, 1, 1}, {0, 0.39215687, 1, 1}, {}, {});
            case ie::object::RobotPortal:
                return setParticleColor({0.5882353, 0.5882353, 0.5882353, 1}, {0.19607843, 0.19607843, 0.29411766, 1}, {}, {});
            case ie::object::OrangeMirrorPortal:
                return setParticleColor({1, 0.5882353, 0, 1}, {1, 0.5882353, 0, 1}, {}, {});
            case ie::object::BlueMirrorPortal:
                return setParticleColor({0, 1, 1, 0.5}, {0, 1, 1, 1}, {}, {0, 0.5, 0, 0});
            case ie::object::GreenSizePortal:
                return setParticleColor({0, 1, 0, 1}, {0, 1, 0, 1}, {0.25, 0, 0.25, 0.5}, {});
            case ie::object::PinkSizePortal:
                return setParticleColor({1, 0, 1, 1}, {1, 0, 1, 1}, {0, 0.5, 0, 0.5}, {});
            case ie::object::OrangeDualPortal:
                return setParticleColor({1, 0.78431374, 0, 1}, {1, 0.39215687, 0, 1}, {0, 0.5, 0, 0.5}, {});
            case ie::object::BlueDualPortal:
                return setParticleColor({0, 0.78431374, 1, 1}, {0, 0.39215687, 1, 1}, {0, 0.5, 0, 0.5}, {});
            case ie::object::LinkedBlueTeleport:
            case ie::object::BlueTeleportPortal:
                return setParticleColor({0, 1, 1, 1}, {0, 0.39215687, 0.5882353, 1}, {}, {});
            case ie::object::LinkedOrangeTeleport:
            case ie::object::OrangeTeleportPortal:
                return setParticleColor({1, 0.78431374, 0, 1}, {1, 0.39215687, 0, 1}, {}, {});
            case ie::object::SpiderPortal:
                return setParticleColor({0.78431374, 0, 1, 1}, {0.78431374, 0, 1, 1}, {}, {});
            case ie::object::SwingPortal:
                return setParticleColor({1, 1, 0, 1}, {1, 0.78431374, 0, 1}, {}, {});
            case ie::object::SlowSpeedPortal:
                return setParticleColor({1, 0.9, 0, 0.8}, {1, 0.9, 0, 0}, {0, 0, 0, 0.2}, {});
            case ie::object::NormalSpeedPortal:
                return setParticleColor({0, 0.8, 1, 0.8}, {0, 0.8, 1, 0}, {0, 0, 0, 0.2}, {});
            case ie::object::FastSpeedPortal:
                return setParticleColor({0, 1, 0.2, 0.8}, {0, 1, 0.2, 0}, {0, 0, 0, 0.2}, {});
            case ie::object::FasterSpeedPortal:
                return setParticleColor({1, 0.6, 1, 0.8}, {1, 0.3, 1, 0}, {0, 0, 0, 0.2}, {});
            case ie::object::FastestSpeedPortal:
                return setParticleColor({1, 0, 0, 0.8}, {1, 0, 0, 0}, {0, 0, 0, 0.2}, {});
        }
    }
};

class $modify(LevelEditorLayer) {
    $register_hooks("show-object-particles");

    $override
    CCArray* createObjectsFromString(gd::string const& objString, bool dontCreateUndo, bool dontShowMaxWarning) {
        // ⏺️ prevent particles appearing when creating objects from string (e.g. custom objects preview)

        // note: dontCreateUndo is always true, so on windows 2.2081 it's been optimized out
        // so garbage data is passed in its place

        if (dontShowMaxWarning) s_dontCreateParticles = true;

        CCArray* ret = LevelEditorLayer::createObjectsFromString(objString, dontCreateUndo, dontShowMaxWarning);
        s_dontCreateParticles = false;

        return ret;
    }

    // overriding claimCustomParticle/unclaimCustomParticle may cause issues i don't know of yet. i assume there's a
    // reason rob implemented different behavior in the editor, but from what i can tell this seems to work fine

    $override
    CCParticleSystemQuad* claimCustomParticle(
        gd::string const& key, ParticleStruct const& particleStruct,
        int zLayer, int zOrder, int uiObject, bool dontAdd
    ) {
        // ⏺️ fix particles immediately disappearing on toggle off

        m_particleCount = 0;

        return GJBaseGameLayer::claimCustomParticle(
            key, particleStruct, zLayer, zOrder, uiObject, dontAdd
        );
    }

    $override
    void unclaimCustomParticle(gd::string const& key, CCParticleSystemQuad* particle) {
        // ⏺️ fix particles immediately disappearing on toggle off

        GJBaseGameLayer::unclaimCustomParticle(key, particle);
    }
};

void ie::updateObjectParticle(LevelEditorLayer* lel, GameObject* object) {
    if (!g_showObjectParticles || !object->m_particle || ie::object::isCustomParticle(object)) return;

    object->m_particle->setVisible(!object->m_hasNoParticles);
}
