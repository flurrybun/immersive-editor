#include "CheckboxPopup.hpp"

bool CheckboxPopup::init(
    ZStringView id, ZStringView info, const std::vector<CheckboxOption>& options, Callback callback
) {
    if (!Popup::init(400.f, 300.f, "square01_001.png", { 0, 0, 94, 94 })) return false;

    m_callback = std::move(callback);

    Mod::get()->setSavedValue(id + "-shown", true);
    m_closeBtn->removeFromParent();

    auto contentSize = CCSize {
        m_size.width - 40.f,
        m_size.height - 35.f,
    };

    auto mainNode = CCNode::create();
    mainNode->setContentSize(contentSize);
    mainNode->setAnchorPoint({ 0.5f, 0.5f });
    mainNode->setLayout(
        ColumnLayout::create()
            ->setAxisReverse(true)
            ->setAutoScale(false)
            ->setCrossAxisOverflow(false)
    );
    m_mainLayer->addChildAtPosition(mainNode, Anchor::Center);

    auto title = CCLabelBMFont::create("NinKaz's Immersive Editor", "goldFont.fnt");
    title->setScale(0.7f);

    auto contentContainer = SpacerNode::create();

    auto optionsContainer = NineSlice::create("square02b_001.png", { 0.f, 0.f, 80.f, 80.f });
    optionsContainer->setScale(0.5f);
    optionsContainer->setColor({ 0, 0, 0 });
    optionsContainer->setOpacity(75);
    optionsContainer->setScaledContentSize({ contentSize.width, contentSize.height / 2.f });

    auto optionsMenu = CCMenu::create();
    optionsMenu->setScale(2.f);
    optionsMenu->setContentSize({ contentSize.width - 30.f, 0.f });
    optionsMenu->setLayout(
        RowLayout::create()
            ->setAxisAlignment(AxisAlignment::Start)
            ->setCrossAxisLineAlignment(AxisAlignment::End)
            ->setGrowCrossAxis(true)
            ->setAutoScale(false)
            ->setGap(7.f)
    );

    for (const auto& option : options) {
        createOption(optionsMenu, option);
    }

    auto disclaimer = CCLabelBMFont::create(
        "(You can change this later in the mod settings)",
        "bigFont.fnt"
    );
    disclaimer->setScale(0.3f);
    disclaimer->setColor({ 64, 227, 72 });
    disclaimer->setAnchorPoint({ 0.f, 0.5f });
    disclaimer->setLayoutOptions(
        AxisLayoutOptions::create()
            ->setBreakLine(true)
    );
    optionsMenu->addChild(disclaimer);

    optionsMenu->updateLayout();
    optionsContainer->setScaledContentSize(optionsMenu->getContentSize() + CCSize{ 30.f, 20.f });
    optionsContainer->addChildAtPosition(optionsMenu, Anchor::Center);

    auto confirmBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("OK"), this, menu_selector(CheckboxPopup::onClose)
    );
    auto disabledSpr = ButtonSprite::create("OK", "goldFont.fnt", "GJ_button_04.png");
    disabledSpr->setCascadeColorEnabled(true);
    disabledSpr->setColor(ccGRAY);
    confirmBtn->setDisabledImage(disabledSpr);

    auto btnMenu = CCMenu::create();
    btnMenu->setLayout(
        SimpleRowLayout::create()
            ->setMainAxisScaling(AxisScaling::Fit)
            ->setCrossAxisScaling(AxisScaling::Fit)
    );
    btnMenu->addChild(confirmBtn);
    btnMenu->updateLayout();

    mainNode->addChild(title);
    mainNode->addChild(contentContainer);
    mainNode->addChild(optionsContainer);
    mainNode->addChild(btnMenu);
    mainNode->updateLayout();

    auto content = MDTextArea::create(info, contentContainer->getContentSize(), false);
    content->setPosition({ 7.5f, 0.f });
    content->setAnchorPoint({ 0.f, 0.f });
    contentContainer->addChild(content);

    m_confirmBtn = confirmBtn;
    updateState();

    return true;
}

void CheckboxPopup::createOption(CCMenu* menu, const CheckboxOption& option) {
    auto checkbox = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(CheckboxPopup::onOption), 1.f
    );
    checkbox->setScale(0.7f);
    checkbox->setUserObject(ObjWrapper<std::string>::create(option.m_id));

    auto node = CCNode::create();
    node->setLayoutOptions(
        AxisLayoutOptions::create()
            ->setBreakLine(true)
    );
    node->setLayout(
        SimpleColumnLayout::create()
            ->setMainAxisScaling(AxisScaling::Grow)
            ->setCrossAxisScaling(AxisScaling::Grow)
            ->setCrossAxisAlignment(CrossAxisAlignment::Start)
            ->setGap(2.f)
    );

    auto spacer = CCNode::create();
    spacer->setContentSize({ 0.f, 2.5f });
    node->addChild(spacer);

    auto label = CCLabelBMFont::create(option.m_label.c_str(), "bigFont.fnt");
    label->setScale(0.4f);
    node->addChild(label);

    if (option.m_description) {
        auto description = MultilineBitmapFont::createWithFont(
            "chatFont.fnt", option.m_description.value(), 1.f, 500.f, { 0.f, 0.f }, 16, true
        );
        description->setScale(.5f);
        description->setOpacity(150);
        description->setLayout(
            SimpleColumnLayout::create()
                ->setMainAxisScaling(AxisScaling::Grow)
                ->setCrossAxisScaling(AxisScaling::Grow)
                ->setCrossAxisAlignment(CrossAxisAlignment::Start)
        );
        description->updateLayout();
        node->addChild(description);
    }

    node->updateLayout();
    menu->addChild(checkbox);
    menu->addChild(node);

    m_checkboxes.push_back(checkbox);
}

void CheckboxPopup::onOption(CCObject* sender) {
    auto checkbox = static_cast<CCMenuItemToggler*>(sender);
    m_selectedID = static_cast<ObjWrapper<std::string>*>(checkbox->getUserObject())->getValue();

    for (auto& checkbox : m_checkboxes) {
        checkbox->toggle(false);
    }

    updateState();
}

void CheckboxPopup::onClose(CCObject* sender) {
    // prevent back key from closing popup prematurely
    if (sender != m_confirmBtn) return;

    Popup::onClose(sender);
    m_callback(m_selectedID ? *m_selectedID : "");
}

void CheckboxPopup::updateState() {
    m_confirmBtn->setEnabled(m_selectedID.has_value());
}
