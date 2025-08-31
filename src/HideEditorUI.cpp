#include <Geode/modify/LevelEditorLayer.hpp>
#include <alphalaneous.good_grid/include/DrawGridAPI.hpp>
#include <alphalaneous.good_grid/include/DrawLayers.hpp>
#include "misc/PlaytestEvent.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(LevelEditorLayer) {
    struct Fields {
        PlaytestEventListener playtestListener;
        bool hidePath = false;
        bool showGround = false;
    };

    $override
    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        DrawGridAPI::get().getNode<Ground>("ground").inspect([](Ground& ground) {
            ground.setEnabled(false);
        });

        DrawGridAPI::get().getNode<PositionLines>("position-lines").inspect([](PositionLines& posLines) {
            posLines.setEnabled(false);
        });

        m_fields->playtestListener.bind([&](PlaytestEvent* event) {
            bool isPlaying = event->isPlaying();

            if (isPlaying) {
                m_fields->hidePath = GameManager::get()->getGameVariable("0152");
                GameManager::get()->setGameVariable("0152", true);

                m_fields->showGround = m_showGround;
                toggleGround(true);
            } else {
                GameManager::get()->setGameVariable("0152", m_fields->hidePath);

                toggleGround(m_fields->showGround);
            }

            DrawGridAPI::get().getNode<Bounds>("bounds").inspect([isPlaying](Bounds& bounds) {
                bounds.setEnabled(!isPlaying);
            });

            DrawGridAPI::get().getNode<BPMTriggers>("bpm-triggers").inspect([isPlaying](BPMTriggers& bpmTriggers) {
                bpmTriggers.setEnabled(!isPlaying);
            });

            return ListenerResult::Propagate;
        });

        return true;
    }
};
