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
using namespace asp::time;

int g_startPosCount = 0;

$on_enable("better-loading-layer") {
    ctx.onObjectEvent([](GameObject* object, bool created) {
        if (!ie::object::isStartPos(object)) return;

        g_startPosCount += created ? 1 : -1;
    });
}

void drawScene() {
    auto dir = CCDirector::get();
    dir->m_bPaused = true;
    dir->drawScene();
    dir->m_bPaused = false;
}

class ProgressTracker {
    using DrawCallback = geode::Function<void(float progress, std::string_view label)>;

    int m_stageCount;
    std::string m_label;
    DrawCallback m_drawCallback;

    int m_currentStage = 0;
    int m_subStageCount = 0;
    int m_currentSubStage = 0;

    Instant m_lastUpdate;

    float getRawProgress() const {
        int maxStage = m_stageCount;

        if (m_currentStage >= maxStage) return 1.f;
        if (m_subStageCount == 0) return static_cast<float>(m_currentStage) / maxStage;

        float progress = (float)m_currentStage / maxStage;

        if (m_subStageCount > 0) {
            float subProgress = static_cast<float>(m_currentSubStage) / m_subStageCount;
            float stageAmount = 1.f / maxStage;

            progress += subProgress * stageAmount;
        }

        return progress;
    }

public:
    ProgressTracker() = default;
    ProgressTracker(int stageCount, std::string label, DrawCallback drawCallback)
        : m_stageCount(stageCount), m_label(std::move(label)), m_drawCallback(std::move(drawCallback)) {}

    void nextStage(std::string label) {
        m_label = std::move(label);
        m_currentStage++;

        m_currentSubStage = 0;
        m_subStageCount = 0;

        m_drawCallback(getProgress(), getLabel());
    }

    void setSubStageCount(int subStageCount) {
        m_subStageCount = subStageCount;
        m_currentSubStage = 0;

        m_lastUpdate = Instant::now();
    }

    void nextSubStage() {
        m_currentSubStage = std::min(m_currentSubStage + 1, m_subStageCount - 1);

        if (m_lastUpdate.elapsed() >= Duration::fromMillis(200)) {
            m_lastUpdate = Instant::now();
            m_drawCallback(getProgress(), getLabel());
        }
    }

    void setSubStage(int count) {
        m_currentSubStage = std::min(count, m_subStageCount - 1);

        if (m_lastUpdate.elapsed() >= Duration::fromMillis(200)) {
            m_lastUpdate = Instant::now();
            m_drawCallback(getProgress(), getLabel());
        }
    }

    float getProgress() const {
        constexpr float BUFFER = 0.1f;

        float progress = BUFFER + getRawProgress() * (1.f - BUFFER * 2.f);
        return progress * 100.f;
    }

    std::string getLabel() const {
        if (m_subStageCount <= 0 || m_currentSubStage <= 0) return m_label;

        int progress = static_cast<float>(m_currentSubStage) / m_subStageCount * 100.f;
        return fmt::format("{} ({}%)", m_label, progress);
    }
};

struct BLLGJGameLoadingLayer;
BLLGJGameLoadingLayer* g_loadingLayer = nullptr;
ProgressTracker* g_progressTracker = nullptr;

class $modify(BLLGJGameLoadingLayer, GJGameLoadingLayer) {
    struct Fields {
        CCNode* container;
        ProgressBar* bar;
        CCLabelBMFont* label;

        ProgressTracker progressTracker;
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
        g_progressTracker = nullptr;
    }

    void setupBetterLoadingLayer(bool isEditor) {
        this->removeAllChildren();
        this->setPosition(CCDirector::get()->getWinSize() / 2.f);

        int numSteps;

        if (isEditor) numSteps = 5;
        else if (g_startPosCount > 0) numSteps = 4;
        else numSteps = 3;

        m_fields->progressTracker = ProgressTracker(
            numSteps,
            "Initializing",
            [this](float progress, std::string_view label) {
                m_fields->bar->updateProgress(progress);
                m_fields->label->setString(label.data());
                m_fields->container->updateLayout();

                drawScene();
            }
        );
        g_progressTracker = &m_fields->progressTracker;

        auto container = CCNode::create();
        container->setLayout(
            SimpleColumnLayout::create()
                ->setMainAxisScaling(AxisScaling::None)
                ->setMainAxisAlignment(MainAxisAlignment::Start)
                ->setCrossAxisAlignment(CrossAxisAlignment::End)
                ->setMainAxisDirection(AxisDirection::BottomToTop)
                ->setGap(2.f)
        );

        auto bar = ProgressBar::create(ProgressBarStyle::Slider);
        bar->updateProgress(m_fields->progressTracker.getProgress());

        auto label = CCLabelBMFont::create("Initializing", "bigFont.fnt");
        label->setScale(0.6f);

        container->addChild(bar);
        container->addChild(label);
        container->updateLayout();

        this->addChildAtPosition(container, Anchor::BottomRight, { -11.f, 10.f });

        m_fields->container = container;
        m_fields->bar = bar;
        m_fields->label = label;
    }
};

class $modify(GJBaseGameLayer) {
    $register_hooks("better-loading-layer");

    $override
    bool init() {
        if (!GJBaseGameLayer::init()) return false;

        if (auto pt = g_progressTracker) pt->nextStage("Loading Level");

        return true;
    }

    $override
    void loadUpToPosition(float position, int order, int channel) {
        if (auto pt = g_progressTracker) {
            pt->nextStage("Loading StartPos");

            float duration = timeForPos({ position, 0.f }, order, channel, false, 0);
            if (duration > 3600.f) duration = 3600.f;

            pt->setSubStageCount(std::ceil(duration / 0.016666668f));
        }

        GJBaseGameLayer::loadUpToPosition(position, order, channel);
    }

    $override
    void processMoveActionsStep(float dt, bool visibleFrame) {
        if (auto pt = g_progressTracker) pt->nextSubStage();
        GJBaseGameLayer::processMoveActionsStep(dt, visibleFrame);
    }
};

class $modify(PlayLayer) {
    $register_hooks("better-loading-layer");

    $override
    void prepareCreateObjectsFromSetup(gd::string& levelString) {
        PlayLayer::prepareCreateObjectsFromSetup(levelString);

        if (auto pt = g_progressTracker) {
            pt->nextStage("Creating Objects");

            pt->setSubStageCount(m_objectStrings.size());
        }
    }

    $override
    void processCreateObjectsFromSetup() {
        if (auto pt = g_progressTracker) pt->setSubStage(m_objectsCreated);
        PlayLayer::processCreateObjectsFromSetup();
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

        for (const auto& [key, hook] : self.m_hooks) {
            ie::addHookForSetting("better-loading-layer", hook);
        }
    }

    $override
    void createObjectsFromSetup(gd::string& levelString) {
        if (auto pt = g_progressTracker) pt->nextStage("Creating Objects");
        LevelEditorLayer::createObjectsFromSetup(levelString);
    }

    $override
    bool init(GJGameLevel* level, bool noUI) {
        if (!LevelEditorLayer::init(level, noUI)) return false;

        if (auto pt = g_progressTracker) pt->nextStage("Loading Mods");

        return true;
    }
};

class $modify(EditorUI) {
    $register_hooks("better-loading-layer");

    $override
    bool init(LevelEditorLayer* editorLayer) {
        if (auto pt = g_progressTracker) pt->nextStage("Loading Editor");
        return EditorUI::init(editorLayer);
    }
};
