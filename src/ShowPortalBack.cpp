#include <Geode/modify/LevelEditorLayer.hpp>
#include "UpdateVisibility.hpp"
#include "misc/ObjectEvent.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

// portal backs are a special object (id 38) with the portal's properties duplicated over, but this method
// wouldn't work in the editor. instead, each portal gets a back sprite that follows its pos/rot/scale/color/opacity

class $modify(SPBLevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        ListenerHandle objectListener;
        std::unordered_map<WeakRef<GameObject>, Ref<CCSprite>> portalBacks;
    };

    $override
    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        m_fields->objectListener = ObjectEvent().listen([this](GameObject* object, bool created) {
            if (!isPortal(object->m_objectID)) return ListenerResult::Propagate;

            if (created) {
                addPortalBack(object);
            } else {
                removePortalBack(object);
            }

            return ListenerResult::Propagate;
        });

        return true;
    }

    void addPortalBack(GameObject* object) {
        auto back = CCSprite::createWithSpriteFrameName(getPortalBackFrameName(object->m_objectID));

        LevelEditorLayer::get()->m_game2LayerB0->addChild(back);
        back->setZOrder(object->getZOrder() - 100);

        m_fields->portalBacks[object] = back;
    }

    void removePortalBack(GameObject* object) {
        auto it = m_fields->portalBacks.find(object);
        if (it == m_fields->portalBacks.end()) return;

        it->second->removeFromParent();
        m_fields->portalBacks.erase(it);
    }

    bool isPortal(int objectID) {
        switch (objectID) {
            case 10: // blue gravity
            case 11: // yellow gravity
            case 12: // cube
            case 13: // ship
            case 45: // orange mirror
            case 46: // blue mirror
            case 47: // ball
            case 99: // green size
            case 101: // pink size
            case 111: // ufo
            case 286: // dual
            case 287: // exit dual
            case 660: // wave
            case 745: // robot
            case 747: // linked blue teleport
            case 2902: // standalone blue teleport
            case 749: // linked orange teleport
            case 2064: // standalone orange teleport
            case 1331: // spider
            case 1933: // swing
            case 2926: // green gravity
                return true;
            default:
                return false;
        }
    }

    const char* getPortalBackFrameName(int objectID) {
        switch (objectID) {
            case 10: return "portal_01_back_001.png"; // blue gravity
            case 11: return "portal_02_back_001.png"; // yellow gravity
            case 12: return "portal_03_back_001.png"; // cube
            case 13: return "portal_04_back_001.png"; // ship
            case 45: return "portal_05_back_001.png"; // orange mirror
            case 46: return "portal_06_back_001.png"; // blue mirror
            case 47: return "portal_07_back_001.png"; // ball
            case 99: return "portal_08_back_001.png"; // green size
            case 101: return "portal_09_back_001.png"; // pink size
            case 111: return "portal_10_back_001.png"; // ufo
            case 286: return "portal_11_back_001.png"; // dual
            case 287: return "portal_12_back_001.png"; // exit dual
            case 660: return "portal_13_back_001.png"; // wave
            case 745: return "portal_14_back_001.png"; // robot
            case 747: // linked blue teleport
            case 2902: // standalone blue teleport
                return "portal_15_back_001.png";
            case 749: // linked orange teleport
            case 2064: // standalone orange teleport
                return "portal_16_back_001.png";
            case 1331: return "portal_17_back_001.png"; // spider
            case 1933: return "portal_18_back_001.png"; // swing
            case 2926: return "portal_19_back_001.png"; // green gravity
            default: return "portal_03_back_001.png"; // cube
        }
    }
};

void ie::updatePortalBacks(LevelEditorLayer* lel) {
    auto& portalBacks = static_cast<SPBLevelEditorLayer*>(lel)->m_fields->portalBacks;

    for (auto it = portalBacks.begin(); it != portalBacks.end(); ) {
        GameObject* portal = it->first.lock();
        CCSprite* back = it->second;

        if (!portal) {
            back->removeFromParent();
            it = portalBacks.erase(it);
            continue;
        }

        ++it;

        back->setPosition(portal->getPosition());
        back->setRotationX(portal->getRotationX());
        back->setRotationY(portal->getRotationY());
        back->setScaleX(portal->getScaleX());
        back->setScaleY(portal->getScaleY());

        back->setVisible(portal->isVisible());
        back->setOpacity(portal->getOpacity());
        back->setColor(portal->getColor());
    }
}
