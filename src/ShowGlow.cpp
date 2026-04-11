#include <Geode/modify/GameObject.hpp>
#include "UpdateVisibility.hpp"
#include "misc/Utils.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(GameObject) {
    $override
    void addGlow(gd::string objectFrameName) {
        if (!ie::inEditor()) {
            GameObject::addGlow(objectFrameName);
            return;
        }

        if (GameManager::get()->m_performanceMode) return;

        bool prevEE = m_editorEnabled;
        m_editorEnabled = false;

        GameObject::addGlow(objectFrameName);

        m_editorEnabled = prevEE;
    }

    $override
    static GameObject* createWithKey(int key) {
        GameObject* object = GameObject::createWithKey(key);
        if (!object) return object;

        if (object->m_editorEnabled) {
            object->addGlow(ObjectToolbox::sharedState()->intKeyToFrame(key));
        }

        return object;
    }

    $override
    void setupCustomSprites(gd::string frameName) {
        if (m_objectType != GameObjectType::Slope || !m_editorEnabled) {
            GameObject::setupCustomSprites(frameName);
            return;
        }

        m_editorEnabled = false;
        GameObject::setupCustomSprites(frameName);
        m_editorEnabled = true;
    }

    $override
    void selectObject(ccColor3B color) {
        // ⏺️ fix glow not becoming selected green color

        bool prevCGC = m_customGlowColor;
        bool prevCCG = m_cantColorGlow;
        m_customGlowColor = false;
        m_cantColorGlow = false;

        GameObject::selectObject(color);

        m_customGlowColor = prevCGC;
        m_cantColorGlow = prevCCG;
    }
};

std::optional<ccColor3B> getSpecialGlowColor(LevelEditorLayer* lel, GameObject* object) {
    switch (object->m_objectID) {
        case 36: // yellow orb
        case 35: // yellow pad
            return ccc3(255, 165, 0);
        case 84: // blue orb
        case 67: // blue pad
            return ccc3(0, 255, 255);
        case 141: // pink orb
        case 140: // pink pad
            return ccc3(255, 0, 255);
        case 1333: // red orb
        case 1332: // red pad
            return ccc3(255, 100, 100);
        case 1330: // black orb
        case 1594: // toggle orb
            return lel->m_lightBGColor;
        case 1022: // green orb
        case 1704: // green dash orb
            return ccc3(25, 255, 25);
        case 1751: // pink dash orb
            return ccc3(200, 0, 255);
        case 3004: // spider orb
        case 3005: // spider pad
            return ccc3(100, 0, 255);
        default:
            return std::nullopt;
    }
}

void ie::updateGlow(LevelEditorLayer* lel, GameObject* object) {
    // some objects like orbs, ice spikes, and certain saws don't inherit their glow color from the object color
    // instead, they use a custom color updated per frame in PlayLayer::updateVisibility

    if (auto glowSprite = object->m_glowSprite) {
        glowSprite->setVisible(!object->m_hasNoGlow);
    }

    if (object->m_isSelected) return;

    std::optional<ccColor3B> specialGlowColor = getSpecialGlowColor(lel, object);

    // id 143 is for breakable blocks, which are a special case
    if (object->m_objectID != 143 && !object->m_customGlowColor && !specialGlowColor) return;

    ccColor3B glowColor;

    if (specialGlowColor) {
        glowColor = *specialGlowColor;
    } else if (object->m_glowColorIsLBG) {
        glowColor = lel->m_effectManager->activeColorForIndex(1007);
    } else {
        glowColor = lel->m_lightBGColor;
    }

    bool prevCCG = object->m_cantColorGlow;
    object->m_cantColorGlow = false;

    object->setGlowColor(glowColor);

    object->m_cantColorGlow = prevCCG;
}
