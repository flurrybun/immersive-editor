#include "Selection.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

ie::SelectionBox ie::SelectionBox::fromObject(LevelEditorLayer* lel, GameObject* object, bool fuzzy) {
    SelectionBox box;

    bool useTextureRect = object->m_useTextureRectForSelection ||
        (!object->m_colorSprite && !object->m_hasCustomChild && !object->m_hasAnimatedChild);

    if (!object->m_hasCustomSize) {
        if (object->m_useObjectRect) {
            CCRect rect = object->getObjectRect();
            CCPoint center = rect.origin + rect.size * 0.5f;

            CCAffineTransform transform = CCAffineTransformMakeIdentity();
            box.m_transform = CCAffineTransformTranslate(transform, center.x, center.y);
            box.m_halfSize = rect.size * 0.5f;

            return box;
        }

        box.m_halfSize = useTextureRect
            ? (object->m_obRect.size * 0.5f)
            : (object->getContentSize() * 0.5f);
    } else {
        box.m_halfSize = object->m_customSize * 0.5f;
    }

    CCAffineTransform transform = CCAffineTransformConcat(
        CCAffineTransformConcat(
            object->nodeToParentTransform(),
            lel->m_objectLayer->nodeToWorldTransform()
        ),
        lel->m_objectLayer->worldToNodeTransform()
    );

    CCPoint offset = object->getContentSize() * 0.5f;

    if (useTextureRect) {
        offset += object->m_obUnflippedOffsetPositionFromCenter;
    }

    box.m_transform = CCAffineTransformTranslate(transform, offset.x, offset.y);

    if (fuzzy) {
        float scaleX = std::sqrt(box.m_transform.a * box.m_transform.a + box.m_transform.b * box.m_transform.b);
        float scaleY = std::sqrt(box.m_transform.c * box.m_transform.c + box.m_transform.d * box.m_transform.d);

        box.m_halfSize.width = std::max(box.m_halfSize.width, FUZZY_RADIUS / scaleX);
        box.m_halfSize.height = std::max(box.m_halfSize.height, FUZZY_RADIUS / scaleY);
    }

    return box;
}

ie::SelectionBox ie::SelectionBox::fromRotatedRect(const CCRect& rect, const CCPoint& pivot, float rotation) {
    CCPoint center = ccp(rect.getMidX(), rect.getMidY());

    float radians = -CC_DEGREES_TO_RADIANS(rotation);
    CCPoint dist = center - pivot;

    CCPoint rotatedCenter = pivot + ccp(
        dist.x * std::cos(radians) - dist.y * std::sin(radians),
        dist.x * std::sin(radians) + dist.y * std::cos(radians)
    );

    SelectionBox box;

    box.m_transform = CCAffineTransformMakeIdentity();
    box.m_transform = CCAffineTransformTranslate(box.m_transform, rotatedCenter.x, rotatedCenter.y);
    box.m_transform = CCAffineTransformRotate(box.m_transform, radians);

    box.m_halfSize = CCSizeMake(rect.size.width * 0.5f, rect.size.height * 0.5f);

    return box;
}

ie::SelectionBox ie::SelectionBox::fromCorners(const std::array<CCPoint, 4>& corners) {
    CCPoint center = ranges::reduce<CCPoint>(
        corners,
        [](CCPoint& acc, CCPoint corner) {
            acc += corner;
        }
    );

    CCPoint axisX = (corners[1] - corners[0]).normalize();
    CCPoint axisY = (corners[3] - corners[0]).normalize();

    SelectionBox box;

    box.m_transform = {
        axisX.x, axisX.y,
        axisY.x, axisY.y,
        center.x, center.y
    };
    box.m_halfSize = CCSize(0, 0);

    for (const auto& corner : corners) {
        CCPoint dist = corner - center;

        box.m_halfSize.width = std::max(box.m_halfSize.width, std::abs(ccpDot(dist, axisX)));
        box.m_halfSize.height = std::max(box.m_halfSize.height, std::abs(ccpDot(dist, axisY)));
    }

    return box;
}

bool ie::SelectionBox::containsPoint(const CCPoint& point) const {
    CCAffineTransform inverse = CCAffineTransformInvert(m_transform);

    CCPoint local = CCPointApplyAffineTransform(point, inverse);
    local = ccp(std::abs(local.x), std::abs(local.y));

    return local <= m_halfSize;
}

bool ie::SelectionBox::intersectsRect(const CCRect& rect) const {
    std::array<CCPoint, 4> rectCorners = {
        ccp(rect.getMinX(), rect.getMinY()),
        ccp(rect.getMaxX(), rect.getMinY()),
        ccp(rect.getMaxX(), rect.getMaxY()),
        ccp(rect.getMinX(), rect.getMaxY())
    };

    std::array<CCPoint, 4> corners = getCorners();
    std::array<CCPoint, 4> axes = getAxes(corners);

    if (separatedOnAxis(rectCorners, corners, { 1.f, 0.f })) return false;
    if (separatedOnAxis(rectCorners, corners, { 0.f, 1.f })) return false;

    for (const auto& axis : axes) {
        if (separatedOnAxis(rectCorners, corners, axis)) return false;
    }

    return true;
}

bool ie::SelectionBox::intersectsBox(const ie::SelectionBox& box) const {
    std::array<CCPoint, 4> cornersA = getCorners();
    std::array<CCPoint, 4> cornersB = box.getCorners();
    std::array<CCPoint, 4> axesA = getAxes(cornersA);
    std::array<CCPoint, 4> axesB = getAxes(cornersB);

    for (const auto& axis : axesA) {
        if (separatedOnAxis(cornersA, cornersB, axis)) return false;
    }

    for (const auto& axis : axesB) {
        if (separatedOnAxis(cornersA, cornersB, axis)) return false;
    }

    return true;
}

void ie::SelectionBox::draw(CCDrawNode* drawNode, const ccColor4F& color) const {
    std::array<CCPoint, 4> corners = getCorners();

    for (size_t i = 0; i < corners.size(); i++) {
        const CCPoint& curr = corners[i];
        const CCPoint& next = corners[(i + 1) % corners.size()];

        drawNode->drawSegment(curr, next, 0.5f, color);
    }
}

std::array<CCPoint, 4> ie::SelectionBox::getCorners() const {
    return {
        CCPointApplyAffineTransform(m_halfSize * ccp(-1.f, -1.f), m_transform),
        CCPointApplyAffineTransform(m_halfSize * ccp(1.f, -1.f), m_transform),
        CCPointApplyAffineTransform(m_halfSize * ccp(1.f, 1.f), m_transform),
        CCPointApplyAffineTransform(m_halfSize * ccp(-1.f, 1.f), m_transform)
    };
}

std::array<CCPoint, 4> ie::SelectionBox::getAxes(const std::array<CCPoint, 4>& corners) {
    std::array<CCPoint, 4> axes;

    for (size_t i = 0; i < corners.size(); i++) {
        const CCPoint& curr = corners[i];
        const CCPoint& next = corners[(i + 1) % corners.size()];
        CCPoint edge = next - curr;

        axes[i] = edge.getPerp().normalize();
    }

    return axes;
}

bool ie::SelectionBox::separatedOnAxis(
    const std::array<CCPoint, 4>& cornersA,
    const std::array<CCPoint, 4>& cornersB,
    const CCPoint& axis
) {
    std::pair<float, float> intervalA = projectCorners(cornersA, axis);
    std::pair<float, float> intervalB = projectCorners(cornersB, axis);

    return intervalA.second < intervalB.first || intervalB.second < intervalA.first;
}

std::pair<float, float> ie::SelectionBox::projectCorners(
    const std::array<CCPoint, 4>& corners, const CCPoint& axis
) {
    float min = std::numeric_limits<float>::max();
    float max = std::numeric_limits<float>::lowest();

    for (const auto& corner : corners) {
        float proj = ccpDot(corner, axis);

        min = std::min(min, proj);
        max = std::max(max, proj);
    }

    return { min, max };
}
