#include <Geode/Geode.hpp>
using namespace geode::prelude;

class _PlayerObject : public PlayerObject {
public:
    void _toggleGhostEffect(GhostType ghostType);
};

void _PlayerObject::_toggleGhostEffect(GhostType ghostType) {
    if (m_ghostType == ghostType) return;
    m_ghostType = ghostType;

    if (m_ghostTrail) {
        m_ghostTrail->stopTrail();
        m_ghostTrail = nullptr;
    }

    if (ghostType != GhostType::Enabled) return;

    m_ghostTrail = GhostTrailEffect::create();
    m_ghostTrail->m_playerObject = this;
    m_ghostTrail->m_playerScale = m_vehicleSize;
    m_ghostTrail->m_opacity = 200.f;

    if (m_iconSprite->getColor() != ccBLACK) {
        m_ghostTrail->doBlendAdditive();
        m_ghostTrail->m_color = m_playerColor1;
    } else {
        m_ghostTrail->m_color = ccBLACK;
    }

    m_ghostTrail->runWithTarget(m_iconSprite, 0.05f, 0.4f, 0.f, 0.6f, false);
    m_ghostTrail->m_objectLayer = PlayLayer::get()->m_objectLayer;
    PlayLayer::get()->m_objectLayer->addChild(m_ghostTrail);
}
