#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include "UpdateVisibility.hpp"
#include "misc/Utils.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(GameObject) {
    $override
    void addGlow(gd::string objectFrameName) {
        if (!LevelEditorLayer::get()) {
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

class $modify(SGLevelEditorLayer, LevelEditorLayer) {
    void updateCustomGlowColor(GameObject* object) {
        // some objects like orbs, ice spikes, and certain saws don't inherit their glow color from the object color
        // instead, they use a custom color updated per frame in PlayLayer::updateVisibility

        if (auto glowSprite = object->m_glowSprite) {
            glowSprite->setVisible(!object->m_hasNoGlow);
        }

        if (object->m_isSelected) return;

        std::optional<ccColor3B> specialGlowColor = getSpecialGlowColor(object);

        // id 143 is for breakable blocks, which are a special case
        if (object->m_objectID != 143 && !object->m_customGlowColor && !specialGlowColor.has_value()) return;

        ccColor3B glowColor = {};

        if (specialGlowColor.has_value()) {
            glowColor = specialGlowColor.value();
        } else if (object->m_glowColorIsLBG) {
            if (ColorActionSprite* lbgAction = m_effectManager->m_colorActionSpriteVector[1007]) {
                glowColor = lbgAction->m_color;
            }
        } else {
            glowColor = m_lightBGColor;
        }

        bool prevCCG = object->m_cantColorGlow;
        object->m_cantColorGlow = false;

        object->setGlowColor(glowColor);

        object->m_cantColorGlow = prevCCG;
    }

    void updateFadingBlock(
        GameObject* object, float rightFadeBound, float leftFadeBound,
        float leftFadeWidth, float rightFadeWidth, const ccColor3B& lbgColor
    ) {
        if (!object->m_isInvisibleBlock || !m_previewMode) return;

        std::string style = Mod::get()->getSettingValue<std::string>("invisible-block-style");
        bool isInGame = m_playbackMode == PlaybackMode::Playing || style == "In-Game";

        float layerOpacity = ie::isObjectLayerVisible(object, this) ? 1.f : 0.2f;

        if (object->m_isSelected) {
            object->setOpacity(layerOpacity * 255.f);
            return;
        }

        if (style == "No Fade") {
            object->setOpacity(layerOpacity * 255.f);
            object->setGlowColor(getMixedColor(lbgColor, m_lightBGColor, 0.9f));
            return;
        }

        if (m_playbackMode != PlaybackMode::Playing) {
            float zoom = m_objectLayer->getScale();

            rightFadeBound /= zoom;
            leftFadeBound /= zoom;
            leftFadeWidth /= zoom * 2.f;
            rightFadeWidth /= zoom * 2.f;
        }

        // i have to decomp PlayLayer::updateInvisibleBlock because it's inlined in android64 of all platforms 😭
        // tested it and this decomp is like 99.999% accurate

        float objX = object->getRealPosition().x;

        if (objX <= m_cameraUnzoomedX) objX += object->m_fadeMargin;
        else objX -= object->m_fadeMargin;

        // compute fade near edges of screen:

        // normally m_gameState.m_cameraPosition2.x
        float cameraX = -m_objectLayer->getPositionX() / m_objectLayer->getScale();
        float cameraCenterX = m_halfCameraWidth + cameraX;

        float fadeFactor;

        if (objX <= cameraCenterX) {
            fadeFactor = 0.014285714f * (m_halfCameraWidth - (cameraCenterX - objX));
        } else {
            fadeFactor = 0.02f * (m_halfCameraWidth - (objX - cameraCenterX));
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
            object->setGlowColor(getMixedColor(lbgColor, m_lightBGColor, ratio));
        } else {
            object->setGlowColor(m_lightBGColor);
        }
    }

    ccColor3B getMixedColor(const ccColor3B& color1, const ccColor3B& color2, float ratio) {
        int r = color1.r * (1 - ratio) + color2.r * ratio;
        int g = color1.g * (1 - ratio) + color2.g * ratio;
        int b = color1.b * (1 - ratio) + color2.b * ratio;

        return ccc3(
            std::clamp(r, 0, 255),
            std::clamp(g, 0, 255),
            std::clamp(b, 0, 255)
        );
    }

    std::optional<ccColor3B> getSpecialGlowColor(GameObject* object) {
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
                return m_lightBGColor;
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
};

ie::GlowContext ie::preUpdateGlow(LevelEditorLayer* lel) {
    ccColor3B bgColor = {};
    ccColor3B lbgColor = {255, 255, 255};

    if (ColorActionSprite* lbgAction = lel->m_effectManager->m_colorActionSpriteVector[1000]) {
        bgColor = lbgAction->m_color;
    }

    if (bgColor.r + bgColor.g + bgColor.b < 150) {
        ColorActionSprite* lbgAction = lel->m_effectManager->m_colorActionSpriteVector[1007];
        if (lbgAction) lbgColor = lbgAction->m_color;
    }

    float screenRight = CCDirector::get()->getScreenRight();
    float playerX = screenRight * 0.5 - 75;

    return {
        playerX + 110,
        playerX,
        screenRight - (playerX + 110) - 90,
        playerX - 30,
        lbgColor
    };
}

void ie::updateGlow(LevelEditorLayer* lel, GameObject* object, const GlowContext& context) {
    static_cast<SGLevelEditorLayer*>(lel)->updateCustomGlowColor(object);
    static_cast<SGLevelEditorLayer*>(lel)->updateFadingBlock(
        object,
        context.rightFadeBound,
        context.leftFadeBound,
        context.leftFadeWidth,
        context.rightFadeWidth,
        context.lbgColor
    );
}
