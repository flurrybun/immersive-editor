#include <Geode/modify/GradientTriggerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/utils/VMTHookManager.hpp>
#include "UpdateVisibility.hpp"
#include "misc/ObjectEvent.hpp"
#include "misc/Utils.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(BGTIGradientTriggerObject, GradientTriggerObject) {
    struct Fields {
        CCLayerGradient* gradient;
    };

    $override
    bool init() {
        if (!GradientTriggerObject::init()) return false;
        if (!m_editorEnabled) return true;

        setDisplayFrame(CCSpriteFrameCache::get()->spriteFrameByName("edit_eGradientBtn_001.png"_spr));

        // haaauuuu~!! cute vmthookmanager!! im taking it home with me!!!!
        auto& vmt = VMTHookManager::get();

        (void)vmt.addHook<
            ResolveC<BGTIGradientTriggerObject>::func(&BGTIGradientTriggerObject::customSetup)
        >(this, "GradientTriggerObject::customSetup");

        (void)vmt.addHook<
            ResolveC<BGTIGradientTriggerObject>::func(&BGTIGradientTriggerObject::setObjectColor)
        >(this, "GradientTriggerObject::setObjectColor");

        (void)vmt.addHook<
            ResolveC<BGTIGradientTriggerObject>::func(&BGTIGradientTriggerObject::setChildColor)
        >(this, "GradientTriggerObject::setChildColor");

        (void)vmt.addHook<
            ResolveC<BGTIGradientTriggerObject>::func(&BGTIGradientTriggerObject::setOpacity)
        >(this, "GradientTriggerObject::setOpacity");

        (void)vmt.addHook<
            ResolveC<BGTIGradientTriggerObject>::func(&BGTIGradientTriggerObject::selectObject)
        >(this, "GradientTriggerObject::selectObject");

        return true;
    }

    void customSetup() {
        EffectGameObject::customSetup();
        if (!m_editorEnabled) return;

        // the custom trigger icon and cclayergradient dont work with batch nodes
        m_addToNodeContainer = true;

        ccColor4B defaultColor = { 255, 255, 255, 255 };
        auto gradient = CCLayerGradient::create(defaultColor, defaultColor, { 1.f, 0.f });

        gradient->setContentSize({ 17.3f, 17.3f });
        gradient->ignoreAnchorPointForPosition(false);
        gradient->setZOrder(-1);

        addChildAtPosition(gradient, Anchor::Center, { 0.f, -5.25f });
        m_fields->gradient = gradient;
    }

    void setObjectColor(const ccColor3B& color) {
        if (!m_isSelected) setColor({ 255, 255, 255 });

        m_fields->gradient->setStartColor(color);
    }

    void setChildColor(const ccColor3B& color) {
        m_fields->gradient->setEndColor(color);
    }

    void setOpacity(unsigned char) {
        bool isObjectLayerVisible = ie::isObjectLayerVisible(this, LevelEditorLayer::get());
        float opacityMod = isObjectLayerVisible ? 1.f : (50.f / 255.f);

        CCSprite::setOpacity(isObjectLayerVisible ? 255 : 50);
        m_fields->gradient->setStartOpacity(m_baseColor->m_opacity * 255.f * opacityMod);
        m_fields->gradient->setEndOpacity(m_detailColor->m_opacity * 255.f * opacityMod);

        m_colorSprite->setVisible(false);
    }

    void selectObject(ccColor3B color) {
        GameObject::selectObject(color);

        setColor(color);
        setChildColor(color);
    }

    void updateGradientBlendMode() {
        if (!m_fields->gradient) return;

        switch (m_blendingMode) {
            case 0: // normal
                m_fields->gradient->setBlendFunc({ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA });
                break;
            case 1: // additive
                m_fields->gradient->setBlendFunc({ GL_SRC_ALPHA, GL_ONE });
                break;
            case 2: // multiply
                m_fields->gradient->setBlendFunc({ GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA });
                break;
            case 3: // invert
                m_fields->gradient->setBlendFunc({ GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR });
                break;
        }

        m_fields->gradient->setShouldPremultiply(m_blendingMode == 2 || m_blendingMode == 3);
    }
};

void ie::updateGradientTrigger(GameObject* object) {
    if (object->m_objectID != 2903) return;

    static_cast<BGTIGradientTriggerObject*>(object)->updateGradientBlendMode();
}

class $modify(GJBaseGameLayer) {
    $override
    void updateGradientLayers() {
        GJBaseGameLayer::updateGradientLayers();

        if (!LevelEditorLayer::get()) return;

        if (auto object = EditorUI::get()->m_selectedObject) {
            if (object->m_objectID != 2903) return;

            auto trigger = static_cast<GradientTriggerObject*>(object);
            auto gradient = static_cast<GJGradientLayer*>(
                m_gradientLayers->objectForKey(trigger->m_gradientID)
            );

            if (!gradient) return;

            auto color = object->getColor();
            gradient->setStartColor(color);
            gradient->setEndColor(color);

            return;
        }

        for (const auto& object : CCArrayExt<GameObject*>(EditorUI::get()->m_selectedObjects)) {
            if (object->m_objectID != 2903) continue;

            auto trigger = static_cast<GradientTriggerObject*>(object);
            auto gradient = static_cast<GJGradientLayer*>(
                m_gradientLayers->objectForKey(trigger->m_gradientID)
            );

            if (!gradient) continue;

            auto color = object->getColor();
            gradient->setStartColor(color);
            gradient->setEndColor(color);
        }
    }
};
