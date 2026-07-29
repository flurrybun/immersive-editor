#pragma once

#include <Geode/Geode.hpp>
using namespace geode::prelude;

/**
 * Represents an option in a CheckboxPopup. Contains a unique ID, a label, and an optional description.
 */
struct CheckboxOption {
    std::string m_id;
    std::string m_label;
    std::optional<std::string> m_description;

    CheckboxOption(ZStringView id, ZStringView label, std::optional<ZStringView> description = std::nullopt)
        : m_id(id), m_label(label), m_description(description) {}
};

/**
 * A popup with a description, multiple radio buttons, and a confirm button.
 * Used to present multiple options to the user.
 */
class CheckboxPopup : public Popup {
protected:
    using Callback = geode::Function<void(ZStringView id)>;

    Callback m_callback;
    std::optional<std::string> m_selectedID;
    std::vector<CCMenuItemToggler*> m_checkboxes;
    CCMenuItemSpriteExtra* m_confirmBtn;

    bool init(ZStringView id, ZStringView info, const std::vector<CheckboxOption>& options, Callback callback);
    void createOption(CCMenu* menu, const CheckboxOption& option);

    void onOption(CCObject* sender);
    void onClose(CCObject* sender) override;

    void updateState();

public:
    /**
     * Creates a CheckboxPopup. Does not show it.
     * @param id A unique ID for this popup, used for saving if the popup has been shown.
     * @param info A short description.
     * @param options A list of options.
     * @param callback A function called when the user presses the confirm button.
     */
    static CheckboxPopup* create(
        ZStringView id, ZStringView info, const std::vector<CheckboxOption>& options, Callback callback
    ) {
        auto popup = new CheckboxPopup;
        if (popup->init(id, info, options, std::move(callback))) {
            popup->autorelease();
            return popup;
        }
        delete popup;
        return nullptr;
    }

    /**
     * Creates a CheckboxPopup and shows it, but only if the user hasn't already seen it before.
     * @param id A unique ID for this popup, used for saving if the popup has been shown.
     * @param info A short description.
     * @param options A list of options.
     * @param callback A function called when the user presses the confirm button.
     */
    static void createAndShowOnce(
        ZStringView id, ZStringView info, const std::vector<CheckboxOption>& options, Callback callback
    ) {
        if (Mod::get()->getSavedValue<bool>(id + "-shown")) return;
        CheckboxPopup::create(id, info, options, std::move(callback))->show();
    }
};
