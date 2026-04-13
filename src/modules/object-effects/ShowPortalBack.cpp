#include "core/SettingManager.hpp"
#include "core/UpdateVisibility.hpp"
#include "events/ObjectEvent.hpp"
#include "util/ObjectIDs.hpp"

#include <Geode/modify/LevelEditorLayer.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

$bind_setting(g_showPortalBacks, "show-portal-backs");

// portal backs are a special object (id 38) with the portal's properties duplicated over, but this method
// wouldn't work in the editor. instead, each portal gets a back sprite that follows its pos/rot/scale/color/opacity

class $modify(SPBLevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        ListenerHandle objectListener;
        std::unordered_map<WeakRef<GameObject>, Ref<CCSprite>> portalBacks;
    };

    $register_hooks("show-portal-backs");

    $override
    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        m_fields->objectListener = ObjectEvent().listen([this](GameObject* object, bool created) {
            if (!ie::object::isPortal(object)) return ListenerResult::Propagate;

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

        m_game2LayerB0->addChild(back);
        back->setZOrder(object->getZOrder() - 100);

        m_fields->portalBacks[object] = back;
    }

    void removePortalBack(GameObject* object) {
        auto it = m_fields->portalBacks.find(object);
        if (it == m_fields->portalBacks.end()) return;

        it->second->removeFromParent();
        m_fields->portalBacks.erase(it);
    }

    const char* getPortalBackFrameName(int objectID) {
        switch (objectID) {
            case ie::object::BlueGravityPortal:
                return "portal_01_back_001.png";
            case ie::object::YellowGravityPortal:
                return "portal_02_back_001.png";
            case ie::object::CubePortal:
                return "portal_03_back_001.png";
            case ie::object::ShipPortal:
                return "portal_04_back_001.png";
            case ie::object::OrangeMirrorPortal:
                return "portal_05_back_001.png";
            case ie::object::BlueMirrorPortal:
                return "portal_06_back_001.png";
            case ie::object::BallPortal:
                return "portal_07_back_001.png";
            case ie::object::GreenSizePortal:
                return "portal_08_back_001.png";
            case ie::object::PinkSizePortal:
                return "portal_09_back_001.png";
            case ie::object::UfoPortal:
                return "portal_10_back_001.png";
            case ie::object::OrangeDualPortal:
                return "portal_11_back_001.png";
            case ie::object::BlueDualPortal:
                return "portal_12_back_001.png";
            case ie::object::WavePortal:
                return "portal_13_back_001.png";
            case ie::object::RobotPortal:
                return "portal_14_back_001.png";
            case ie::object::LinkedBlueTeleport:
            case ie::object::BlueTeleportPortal:
                return "portal_15_back_001.png";
            case ie::object::LinkedOrangeTeleport:
            case ie::object::OrangeTeleportPortal:
                return "portal_16_back_001.png";
            case ie::object::SpiderPortal:
                return "portal_17_back_001.png";
            case ie::object::SwingPortal:
                return "portal_18_back_001.png";
            case ie::object::GreenGravityPortal:
                return "portal_19_back_001.png";
            default: return "portal_03_back_001.png";
        }
    }
};

void ie::updatePortalBacks(LevelEditorLayer* lel) {
    auto& portalBacks = static_cast<SPBLevelEditorLayer*>(lel)->m_fields->portalBacks;

    if (!g_showPortalBacks) {
        if (portalBacks.empty()) return;

        for (auto& [_, back] : portalBacks) {
            back->removeFromParent();
        }

        portalBacks.clear();
        return;
    }

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
