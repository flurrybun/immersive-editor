#pragma once

#include <Geode/loader/Dispatch.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/cocos/include/ccTypes.h>

#ifdef MY_MOD_ID
    #undef MY_MOD_ID
#endif
#define MY_MOD_ID "ninkaz.immersive_editor"

#ifndef GEODE_DEFINE_EVENT_EXPORTS
#define $_export(...) GEODE_EVENT_EXPORT_NORES(__VA_ARGS__)
#else
#define $_export(...)
#endif

namespace ninkaz {
    /**
     * Represents the selection hitbox of a GameObject as an OBB.
     * @note All points are in m_objectLayer space
     */
    class SelectionBox {
    public:
        /**
         * Creates a SelectionBox for a GameObject.
         * @param lel The current editor layer
         * @param object The object to create a selection box for
         * @param fuzzy Whether to apply a minimum size to make it easier to select at position
         */
        static SelectionBox fromObject(LevelEditorLayer* lel, GameObject* object, bool fuzzy)
            $_export(&SelectionBox::fromObject, (lel, object, fuzzy));

        /**
         * Creates a SelectionBox from a rectangle, pivot and rotation.
          * @param rect The rectangle to create the box from
          * @param pivot The pivot point to rotate around
          * @param rotation The rotation in degrees
         */
        static SelectionBox fromRect(const cocos2d::CCRect& rect, const cocos2d::CCPoint& pivot, float rotation)
            $_export(&SelectionBox::fromRect, (rect, pivot, rotation));

        /**
         * Creates a SelectionBox that encompasses the four corners.
         * @param corners The corners of the box, in clockwise or counter-clockwise order
         * @note The corners must form a parallelogram
         */
        static SelectionBox fromCorners(const std::array<cocos2d::CCPoint, 4>& corners)
            $_export(&SelectionBox::fromCorners, (corners));

        /**
         * Checks if the box contains a point.
          * @param point The point to check
          * @return Whether the point is inside the box
         */
        bool containsPoint(const cocos2d::CCPoint& point) const
            $_export(&SelectionBox::containsPoint, (this, point));

        /**
         * Checks if the box intersects with a rect.
         * @param rect The rectangle to check
         * @return Whether the box intersects with the rect
         */
        bool intersectsRect(const cocos2d::CCRect& rect) const
            $_export(&SelectionBox::intersectsRect, (this, rect));

        /**
         * Checks if the box intersects with another box.
         * @param box The box to check
         * @return Whether the two boxes intersect
         */
        bool intersectsBox(const SelectionBox& box) const
            $_export(&SelectionBox::intersectsBox, (this, box));

        /**
         * Draws the box using a CCDrawNode.
         * @param drawNode The CCDrawNode to draw with (such as m_debugDrawNode)
         * @param color The color to draw the box with
         */
        void draw(cocos2d::CCDrawNode* drawNode, const cocos2d::ccColor4F& color = { 0.f, 1.f, 0.f, 1.f }) const
            $_export(&SelectionBox::draw, (this, drawNode, color));

    private:
        SelectionBox() = default;

        static constexpr float FUZZY_RADIUS = 4.f;
        cocos2d::CCAffineTransform m_transform;
        cocos2d::CCSize m_halfSize;

        std::array<cocos2d::CCPoint, 4> getCorners() const;
        static std::array<cocos2d::CCPoint, 4> getAxes(const std::array<cocos2d::CCPoint, 4>& corners);
        static bool separatedOnAxis(
            const std::array<cocos2d::CCPoint, 4>& cornersA,
            const std::array<cocos2d::CCPoint, 4>& cornersB,
            const cocos2d::CCPoint& axis
        );
        static std::pair<float, float> projectCorners(
            const std::array<cocos2d::CCPoint, 4>& corners,
            const cocos2d::CCPoint& axis
        );
    };

    #ifdef GEODE_DEFINE_EVENT_EXPORTS
        GEODE_EVENT_EXPORT_NORES(&SelectionBox::fromObject, (lel, object, fuzzy));
        GEODE_EVENT_EXPORT_NORES(&SelectionBox::fromRect, (rect, pivot, rotation));
        GEODE_EVENT_EXPORT_NORES(&SelectionBox::fromCorners, (corners));
        GEODE_EVENT_EXPORT_NORES(&SelectionBox::containsPoint, (this, point));
        GEODE_EVENT_EXPORT_NORES(&SelectionBox::intersectsRect, (this, rect));
        GEODE_EVENT_EXPORT_NORES(&SelectionBox::intersectsBox, (this, box));
        GEODE_EVENT_EXPORT_NORES(&SelectionBox::draw, (this, drawNode, color));
    #endif
}

#undef $_export
