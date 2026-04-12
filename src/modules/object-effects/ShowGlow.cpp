#include "core/SettingManager.hpp"
#include "core/UpdateVisibility.hpp"
#include "util/Utils.hpp"

#include <Geode/modify/GameObject.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

$bind_setting(g_showGlow, "show-glow");

class $modify(GameObject) {
    $register_hooks("show-glow");

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

void updateInvisibleBlock(LevelEditorLayer* lel, GameObject* object, const ie::GlowContext& context) {
    if (!object->m_isInvisibleBlock || !lel->m_previewMode) return;

    std::string style = Mod::get()->getSettingValue<std::string>("invisible-block-style");
    bool isInGame = lel->m_playbackMode == PlaybackMode::Playing || style == "In-Game";

    float layerOpacity = ie::isObjectLayerVisible(object, lel) ? 1.f : 0.2f;

    if (object->m_isSelected) {
        object->setOpacity(layerOpacity * 255.f);
        return;
    }

    const ccColor3B& lbgColor = context.lbgColor;
    float rightFadeBound = context.rightFadeBound;
    float leftFadeBound = context.leftFadeBound;
    float leftFadeWidth = context.leftFadeWidth;
    float rightFadeWidth = context.rightFadeWidth;

    if (style == "No Fade") {
        object->setOpacity(layerOpacity * 255.f);
        object->setGlowColor(GJEffectManager::getMixedColor(lel->m_lightBGColor, lbgColor, 0.9f));
        return;
    }

    if (lel->m_playbackMode != PlaybackMode::Playing) {
        float zoom = lel->m_objectLayer->getScale();

        rightFadeBound /= zoom;
        leftFadeBound /= zoom;
        leftFadeWidth /= zoom * 2.f;
        rightFadeWidth /= zoom * 2.f;
    }

    // decomp of PlayLayer::updateInvisibleBlock but with some extra stuff

    float objX = object->getRealPosition().x;

    if (objX <= lel->m_cameraUnzoomedX) objX += object->m_fadeMargin;
    else objX -= object->m_fadeMargin;

    // compute fade near edges of screen:

    // normally m_gameState.m_cameraPosition2.x
    float cameraX = -lel->m_objectLayer->getPositionX() / lel->m_objectLayer->getScale();
    float cameraCenterX = lel->m_halfCameraWidth + cameraX;

    float fadeFactor;

    if (objX <= cameraCenterX) {
        fadeFactor = 0.014285714f * (lel->m_halfCameraWidth - (cameraCenterX - objX));
    } else {
        fadeFactor = 0.02f * (lel->m_halfCameraWidth - (objX - cameraCenterX));
    }

    float cameraFade = std::clamp(fadeFactor, 0.f, 1.f) * 255.f;

    // compute fade near center of screen:

    float distance;
    float divisor;

    if (objX <= cameraX + rightFadeBound) {
        distance = (cameraX + leftFadeBound) - objX;
        divisor = leftFadeWidth;
    } else {
        distance = objX - cameraX - rightFadeBound;
        divisor = rightFadeWidth;
    }

    if (divisor <= 1.f) divisor = 1.f;

    // normally fixed at 0.05
    float minOpacity = isInGame ? 0.05f : 0.3f;

    float ratio = std::clamp(distance / divisor, 0.f, 1.f);
    float playerFade = (ratio * (1.f - minOpacity) + minOpacity) * 255.f;

    // set final opacity based on both fades:

    object->setOpacity(std::min(cameraFade, playerFade) * layerOpacity);

    // set glow opacity and color:

    if (object->m_glowSprite) {
        // normally fixed at 0.15
        float minOpacity = isInGame ? 0.15f : 0.25f;

        float glowFade = (ratio * (1.f - minOpacity) + minOpacity) * 255.f;
        glowFade = std::min(cameraFade, glowFade);

        GLubyte opacity = glowFade * object->m_opacityMod * layerOpacity;

        object->m_glowSprite->setOpacity(opacity);
        object->m_glowSprite->setChildOpacity(opacity);
    }

    float opacity = object->getOpacity() / 255.f;

    if (opacity > 0.8f) {
        float ratio = (1.0f - (opacity - 0.8f) / 0.2f) * 0.3f + 0.7f;
        object->setGlowColor(GJEffectManager::getMixedColor(lel->m_lightBGColor, lbgColor, ratio));
    } else {
        object->setGlowColor(lel->m_lightBGColor);
    }
}

ie::GlowContext ie::preUpdateGlow(LevelEditorLayer* lel) {
    if (!g_showGlow) return {};

    ccColor3B bgColor = lel->m_effectManager->activeColorForIndex(1000);
    int bgColorSum = bgColor.r + bgColor.g + bgColor.b;

    ccColor3B lbgColor = bgColorSum >= 150 ? ccWHITE : lel->m_effectManager->activeColorForIndex(1007);

    float screenRight = CCDirector::get()->getScreenRight();
    float playerX = screenRight * 0.5f - 75.f;

    return {
        playerX + 110.f,
        playerX,
        screenRight - (playerX + 110.f) - 90.f,
        playerX - 30.f,
        lbgColor
    };
}

void ie::updateGlow(LevelEditorLayer* lel, GameObject* object, const ie::GlowContext& context) {
    if (!g_showGlow) return;

    if (object->m_isInvisibleBlock) {
        updateInvisibleBlock(lel, object, context);
    }

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
