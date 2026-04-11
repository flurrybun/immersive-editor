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

namespace ie {
    /**
     * Represents the selection hitbox of a GameObject as an OBB.
     * @note All points are in m_objectLayer space
     */
    class SelectionBox final {
    public:
        /**
         * Creates a SelectionBox for a GameObject.
         * @param lel The current editor layer
         * @param object The object to create a selection box for
         */
        static const SelectionBox& fromObject(LevelEditorLayer* lel, GameObject* object)
            $_export(&SelectionBox::fromObject, (lel, object));

        /**
         * Creates a SelectionBox from a rectangle, pivot and rotation.
         * @param rect The rectangle to create the box from
         * @param pivot The pivot point to rotate around
         * @param rotation The rotation in degrees
         */
        static SelectionBox fromRotatedRect(const cocos2d::CCRect& rect, const cocos2d::CCPoint& pivot, float rotation)
            $_export(&SelectionBox::fromRotatedRect, (rect, pivot, rotation));

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
         * @param fuzzy Whether to apply a minimum size to make it easier to select at position (normally true)
         * @return Whether the point is inside the box
         */
        bool containsPoint(const cocos2d::CCPoint& point, bool fuzzy) const
            $_export(&SelectionBox::containsPoint, (this, point, fuzzy));

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
        SelectionBox() = delete;
        SelectionBox(cocos2d::CCAffineTransform transform, cocos2d::CCSize halfSize);

        static constexpr float FUZZY_RADIUS = 4.f;
        cocos2d::CCAffineTransform m_transform;
        cocos2d::CCSize m_halfSize;
        cocos2d::CCSize m_fuzzyHalfSize;

        const cocos2d::CCSize& getHalfSize(bool fuzzy) const;
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

    /**
     * Sets the preview color of an object, used for highlighting objects that will be selected.
     * @param object The object to set the preview color of
     * @param color The selected color, blended with the object's original color
     * @param selecting Whether the object is being selected via a rect, instead of just being hovered over
     * @note This must be called every frame
     */
    void setPreviewColor(GameObject* object, const cocos2d::ccColor3B& color, bool selecting)
        $_export(&setPreviewColor, (object, color, selecting));

    #ifdef GEODE_DEFINE_EVENT_EXPORTS
        GEODE_EVENT_EXPORT(&SelectionBox::fromObject, (lel, object));
        GEODE_EVENT_EXPORT(&SelectionBox::fromRotatedRect, (rect, pivot, rotation));
        GEODE_EVENT_EXPORT(&SelectionBox::fromCorners, (corners));
        GEODE_EVENT_EXPORT(&SelectionBox::containsPoint, (this, point, fuzzy));
        GEODE_EVENT_EXPORT(&SelectionBox::intersectsRect, (this, rect));
        GEODE_EVENT_EXPORT(&SelectionBox::intersectsBox, (this, box));
        GEODE_EVENT_EXPORT(&SelectionBox::draw, (this, drawNode, color));

        GEODE_EVENT_EXPORT(&setPreviewColor, (object, color, selecting));
    #endif
}

#undef $_export
