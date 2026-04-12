#include "core/SettingManager.hpp"

#include <Geode/modify/GJBaseGameLayer.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

// why this bug happens is a bit complicated because start position calculation is complicated
// consider it supplemental reading: https://cdn.discordapp.com/attachments/562406045742268416/1445499499459117118/image.png?ex=693091d0&is=692f4050&hm=e5dd34f59c2bc9f74cfb04c77f9cfed71dac148f06027770f20f126fd64fec48&

$bind_setting(g_fixFollow, "fix-startpos-follow");
$bind_setting(g_fixCameraShader, "fix-startpos-camera-shader");

class $modify(GJBaseGameLayer) {
    struct Fields {
        bool calculatingStartPos = false;
    };

    $override
    void loadUpToPosition(float position, int order, int channel) {
        // this may not be necessary but i am deathly afraid of breaking normal mode

        m_fields->calculatingStartPos = true;
        GJBaseGameLayer::loadUpToPosition(position, order, channel);
        m_fields->calculatingStartPos = false;
    }

    $override
    void processMoveActionsStep(float dt, bool visibleFrame) {
        GJBaseGameLayer::processMoveActionsStep(dt, visibleFrame);

        // fixes camera and shader triggers not updating during startpos calculation

        if (m_fields->calculatingStartPos && g_fixCameraShader) {
            m_gameState.updateTweenActions(dt);
            m_shaderLayer->m_state.updateTweenActions(dt);
        }
    }

    $override
    void moveObjects(CCArray* objects, double dx, double dy, bool lockPlayerY) {
        if (!m_fields->calculatingStartPos || !g_fixFollow) {
            GJBaseGameLayer::moveObjects(objects, dx, dy, lockPlayerY);
            return;
        }

        for (const auto& object : CCArrayExt<GameObject*>(objects)) {
            object->m_lastPosition.x = object->m_positionX;
            object->m_lastPosition.y = object->m_positionY;
        }

        GJBaseGameLayer::moveObjects(objects, dx, dy, lockPlayerY);
    }

    $override
    void claimRotationAction(
        int targetID, int centerID, float& rotation, float& offset, bool ignoreStaticGroups, bool unused
    ) {
        if (!m_fields->calculatingStartPos || !g_fixFollow) {
            GJBaseGameLayer::claimRotationAction(targetID, centerID, rotation, offset, ignoreStaticGroups, unused);
            return;
        }

        auto objects = getGroup(targetID);
        for (const auto& object : CCArrayExt<GameObject*>(objects)) {
            object->m_lastPosition.x = object->m_positionX;
            object->m_lastPosition.y = object->m_positionY;
        }

        GJBaseGameLayer::claimRotationAction(targetID, centerID, rotation, offset, ignoreStaticGroups, unused);
    }
};
