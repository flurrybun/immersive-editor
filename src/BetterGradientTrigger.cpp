#include <Geode/modify/GradientTriggerObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/utils/VMTHookManager.hpp>
#include "misc/ObjectEvent.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(BGTIGradientTriggerObject, GradientTriggerObject) {
    struct Fields {
        CCLayerGradient* gradient;
    };

    $override
    bool init() {
        if (!GradientTriggerObject::init()) return false;

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

        // adds the trigger to a node container rather than a batch node
        m_hasSpecialChild = true;

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
        m_fields->gradient->setStartOpacity(m_baseColor->m_opacity * 255.f);

        m_colorSprite->setVisible(false);
    }

    void setChildColor(const ccColor3B& color) {
        m_fields->gradient->setEndColor(color);
        m_fields->gradient->setEndOpacity(m_detailColor->m_opacity * 255.f);
    }

    void setOpacity(unsigned char opacity) {}

    void selectObject(ccColor3B color) {
        GameObject::selectObject(color);

        setColor(color);
        setChildColor(color);
    }

    void updateGradientBlendMode() {
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

class $modify(LevelEditorLayer) {
    void updateVisibility(float dt) {
        LevelEditorLayer::updateVisibility(dt);

        for (const auto& obj : m_activeObjects) {
            if (obj->m_objectID != 2903) continue;

            static_cast<BGTIGradientTriggerObject*>(obj)->updateGradientBlendMode();
        }
    }
};
