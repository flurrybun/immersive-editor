#include "ButtonHook.hpp"

using namespace geode::prelude;

class ButtonHook : public CCObject {
    SEL_MenuHandler m_selector;
    CCObject* m_listener;
    ie::ButtonHookCallback m_callback;

public:
    void execute(CCObject* sender) {
        m_callback(sender, [this, sender]() {
            (m_listener->*m_selector)(sender);
        });
    }

    static ButtonHook* create(
        CCMenuItem* button,
        ie::ButtonHookCallback callback
    ) {
        auto ret = new ButtonHook();
        ret->autorelease();

        ret->m_callback = std::move(callback);
        ret->m_selector = button->m_pfnSelector;
        ret->m_listener = button->m_pListener;

        // prevent the hook from being gc'd
        button->setUserObject("button-hook"_spr, ret);

        return ret;
    }
};

void ie::hookButton(
    cocos2d::CCMenuItem* button,
    ie::ButtonHookCallback callback
) {
    auto hook = ButtonHook::create(button, std::move(callback));

    button->m_pfnSelector = menu_selector(ButtonHook::execute);
    button->m_pListener = hook;
}
