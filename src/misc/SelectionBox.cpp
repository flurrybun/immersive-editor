#include "SelectionBox.hpp"

SelectionBox::SelectionBox(LevelEditorLayer* lel, GameObject* object, bool fuzzy) {
    bool useTextureRect = object->m_useTextureRectAsSelectHitbox ||
        (!object->m_colorSprite && !object->m_hasCustomChild && !object->m_hasAnimatedChild);

    if (!object->m_hasContentSize) {
        if (object->m_updateCustomContentSize) {
            CCRect rect = object->getObjectRect();
            CCPoint center = rect.origin + rect.size * 0.5f;

            CCAffineTransform transform = CCAffineTransformMakeIdentity();
            m_transform = CCAffineTransformTranslate(transform, center.x, center.y);
            m_halfSize = rect.size * 0.5f;

            return;
        }

        m_halfSize = useTextureRect
            ? (object->m_obRect.size * 0.5f)
            : (object->getContentSize() * 0.5f);
    } else {
        m_halfSize = object->m_lastSize * 0.5f;
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

    m_transform = CCAffineTransformTranslate(transform, offset.x, offset.y);

    if (fuzzy) {
        float scaleX = std::sqrt(m_transform.a * m_transform.a + m_transform.b * m_transform.b);
        float scaleY = std::sqrt(m_transform.c * m_transform.c + m_transform.d * m_transform.d);

        m_halfSize.width = std::max(m_halfSize.width, FUZZY_RADIUS / scaleX);
        m_halfSize.height = std::max(m_halfSize.height, FUZZY_RADIUS / scaleY);
    }
}

bool SelectionBox::containsPoint(const CCPoint& point) const {
    CCAffineTransform inverse = CCAffineTransformInvert(m_transform);

    CCPoint local = CCPointApplyAffineTransform(point, inverse);
    local = ccp(std::abs(local.x), std::abs(local.y));

    return local <= m_halfSize;
}

bool SelectionBox::intersectsRect(const CCRect& rect) const {
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

void SelectionBox::draw(CCDrawNode* drawNode) const {
    std::array<CCPoint, 4> corners = getCorners();

    for (size_t i = 0; i < corners.size(); i++) {
        const CCPoint& curr = corners[i];
        const CCPoint& next = corners[(i + 1) % corners.size()];

        drawNode->drawSegment(curr, next, 0.5f, { 0.f, 1.f, 0.f, 1.f });
    }
}

std::array<CCPoint, 4> SelectionBox::getCorners() const {
    return {
        CCPointApplyAffineTransform(m_halfSize * ccp(-1.f, -1.f), m_transform),
        CCPointApplyAffineTransform(m_halfSize * ccp(1.f, -1.f), m_transform),
        CCPointApplyAffineTransform(m_halfSize * ccp(1.f, 1.f), m_transform),
        CCPointApplyAffineTransform(m_halfSize * ccp(-1.f, 1.f), m_transform)
    };
}

std::array<CCPoint, 4> SelectionBox::getAxes(const std::array<CCPoint, 4>& corners) const {
    std::array<CCPoint, 4> axes;

    for (size_t i = 0; i < corners.size(); i++) {
        const CCPoint& curr = corners[i];
        const CCPoint& next = corners[(i + 1) % corners.size()];
        CCPoint edge = next - curr;

        axes[i] = edge.getPerp().normalize();
    }

    return axes;
}

bool SelectionBox::separatedOnAxis(
    const std::array<CCPoint, 4>& cornersA,
    const std::array<CCPoint, 4>& cornersB,
    const CCPoint& axis
) {
    std::pair<float, float> intervalA = projectCorners(cornersA, axis);
    std::pair<float, float> intervalB = projectCorners(cornersB, axis);

    return intervalA.second < intervalB.first || intervalB.second < intervalA.first;
}

std::pair<float, float> SelectionBox::projectCorners(
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
