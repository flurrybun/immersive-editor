#pragma once

#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/cocos/include/ccTypes.h>

using namespace geode::prelude;

/**
 * Represents the selection hitbox of a GameObject as an OBB.
 * @note All points are in m_objectLayer space
 */
class SelectionBox {
public:
    SelectionBox(LevelEditorLayer* lel, GameObject* object, bool fuzzy);

    bool containsPoint(const CCPoint& point) const;
    bool intersectsRect(const CCRect& rect) const;
    void draw(CCDrawNode* drawNode, const ccColor4F& color = {0.f, 1.f, 0.f, 1.f}) const;

private:
    static constexpr float FUZZY_RADIUS = 4.f;
    CCAffineTransform m_transform;
    CCSize m_halfSize;

    std::array<CCPoint, 4> getCorners() const;
    std::array<CCPoint, 4> getAxes(const std::array<CCPoint, 4>& corners) const;

    static bool separatedOnAxis(
        const std::array<CCPoint, 4>& cornersA, const std::array<CCPoint, 4>& cornersB, const CCPoint& axis
    );
    static std::pair<float, float> projectCorners(const std::array<CCPoint, 4>& corners, const CCPoint& axis);
};
