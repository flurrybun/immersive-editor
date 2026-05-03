#include "Geode/modify/Modify.hpp"
#include "core/SettingManager.hpp"
#include "util/ObjectIDs.hpp"

#include <Geode/modify/GJGameLoadingLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

struct BLLGJGameLoadingLayer;
BLLGJGameLoadingLayer* g_loadingLayer = nullptr;

int g_startPosCount = 0;

$on_enable("better-loading-layer") {
    ctx.onObjectEvent([](GameObject* object, bool created) {
        if (!ie::object::isStartPos(object)) return;

        g_startPosCount += created ? 1 : -1;
    });
}

class $modify(BLLGJGameLoadingLayer, GJGameLoadingLayer) {
    struct Fields {
        CCNode* container;
        ProgressBar* bar;
        CCLabelBMFont* label;

        float progressStep;
    };

    $register_hooks("better-loading-layer");

    $override
    static GJGameLoadingLayer* transitionToLoadingLayer(GJGameLevel* level, bool isEditor) {
        GJGameLoadingLayer* ret = GJGameLoadingLayer::transitionToLoadingLayer(level, isEditor);

        // GJGameLoadingLayer::init is inlined

        if (ret) {
            auto modRet = static_cast<BLLGJGameLoadingLayer*>(ret);

            modRet->setupBetterLoadingLayer(isEditor);
            g_loadingLayer = modRet;
        }

        return ret;
    }

    $override
    void loadLevel() {
        GJGameLoadingLayer::loadLevel();
        g_loadingLayer = nullptr;
    }

    void setupBetterLoadingLayer(bool isEditor) {
        this->removeAllChildren();
        this->setPosition(CCDirector::get()->getWinSize() / 2.f);

        auto container = CCNode::create();
        container->setLayout(
            SimpleColumnLayout::create()
                ->setMainAxisScaling(AxisScaling::None)
                ->setMainAxisAlignment(MainAxisAlignment::Start)
                ->setCrossAxisAlignment(CrossAxisAlignment::End)
                ->setMainAxisDirection(AxisDirection::BottomToTop)
                ->setGap(2.f)
        );

        constexpr float BUFFER = 10.f;

        auto bar = ProgressBar::create(ProgressBarStyle::Slider);
        bar->updateProgress(BUFFER);

        auto label = CCLabelBMFont::create("Initializing", "bigFont.fnt");
        label->setScale(0.6f);

        container->addChild(bar);
        container->addChild(label);
        container->updateLayout();

        this->addChildAtPosition(container, Anchor::BottomRight, { -11.f, 10.f });

        m_fields->container = container;
        m_fields->bar = bar;
        m_fields->label = label;

        int numSteps;

        if (isEditor) numSteps = 5;
        else if (g_startPosCount > 0) numSteps = 4;
        else numSteps = 3;

        m_fields->progressStep = (100.f - BUFFER * 2.f) / (numSteps - 1);
    }

    void setProgressLabel(std::string_view text) {
        m_fields->label->setString(text.data());
        m_fields->container->updateLayout();

        float progress = m_fields->bar->getProgress() + m_fields->progressStep;
        m_fields->bar->updateProgress(progress);
    }
};

void setProgressLabel(std::string_view text) {
    if (!g_loadingLayer) return;

    g_loadingLayer->setProgressLabel(text);

    // partial decomp of CCDirector::drawScene, but without some
    // unnecessary stuff like calling CCScheduler::update

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    kmGLPushMatrix();
    g_loadingLayer->visit();
    kmGLPopMatrix();
    CCDirector::sharedDirector()->m_pobOpenGLView->swapBuffers();
}

class $modify(GJBaseGameLayer) {
    $register_hooks("better-loading-layer");

    $override
    bool init() {
        if (!GJBaseGameLayer::init()) return false;

        setProgressLabel("Loading Level");

        return true;
    }

    void loadStartPosObject() {
        setProgressLabel("Loading StartPos");
        GJBaseGameLayer::loadStartPosObject();
    }
};

class $modify(PlayLayer) {
    $register_hooks("better-loading-layer");

    $override
    void prepareCreateObjectsFromSetup(gd::string& levelString) {
        setProgressLabel("Creating Objects");
        PlayLayer::prepareCreateObjectsFromSetup(levelString);
    }

    $override
    void resetLevel() {
        PlayLayer::resetLevel();

        if (m_startPosObject) {
            g_startPosCount = 1;
        }
    }
};

class $modify(LevelEditorLayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("LevelEditorLayer::init", Priority::FirstPost);

        for (const auto &[key, hook] : self.m_hooks) {
            ie ::addHookForSetting("better-loading-layer", hook);
        }
    }

    $override
    void createObjectsFromSetup(gd::string& levelString) {
        setProgressLabel("Creating Objects");
        LevelEditorLayer::createObjectsFromSetup(levelString);
    }

    $override
    bool init(GJGameLevel* level, bool noUI) {
        if (!LevelEditorLayer::init(level, noUI)) return false;

        setProgressLabel("Loading Mods");

        return true;
    }
};

class $modify(EditorUI) {
    $register_hooks("better-loading-layer");

    $override
    bool init(LevelEditorLayer* editorLayer) {
        setProgressLabel("Loading Editor");
        return EditorUI::init(editorLayer);
    }
};
