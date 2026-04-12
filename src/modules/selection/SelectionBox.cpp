#include "events/ObjectEvent.hpp"
#include "modules/selection/Selection.hpp"

#include <Geode/modify/LevelEditorLayer.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

// using a profiler, i found SelectionBox::fromObject to be a big source of lag in levels with
// many objects. rather than recalculating hundreds of selection boxes per frame, they're cached
// and only recalculated when its object's position, scale, or rotation changes

struct CachedTransform {
    float x, y;
    float scaleX, scaleY;
    float rotationX, rotationY;

    bool operator==(const CachedTransform& other) const {
        return x == other.x && y == other.y &&
            scaleX == other.scaleX && scaleY == other.scaleY &&
            rotationX == other.rotationX && rotationY == other.rotationY;
    }

    CachedTransform(GameObject* object) {
        x = object->getPositionX();
        y = object->getPositionY();
        scaleX = object->getScaleX();
        scaleY = object->getScaleY();
        rotationX = object->getRotationX();
        rotationY = object->getRotationY();
    }
};

struct CachedSelectionBox {
    ie::SelectionBox box;
    CachedTransform transform;

    CachedSelectionBox(GameObject* object, ie::SelectionBox box)
        : transform(object), box(std::move(box)) {}
};

class $modify(SBLevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        ListenerHandle objectListener;
        std::unordered_map<GameObject*, CachedSelectionBox> selectionBoxCache;
    };

    $override
    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelEditorLayer::init(p0, p1)) return false;

        m_fields->objectListener = ObjectEvent().listen([this](GameObject* object, bool created) {
            if (created) return ListenerResult::Propagate;

            m_fields->selectionBoxCache.erase(object);

            return ListenerResult::Propagate;
        });

        return true;
    }
};

std::unordered_map<GameObject*, CachedSelectionBox>& getSelectionBoxCache(LevelEditorLayer* lel) {
    return static_cast<SBLevelEditorLayer*>(lel)->m_fields->selectionBoxCache;
}

const ie::SelectionBox& addToCache(LevelEditorLayer* lel, GameObject* object, ie::SelectionBox&& box) {
    auto& cache = getSelectionBoxCache(lel);

    if (cache.size() > std::max(3000, lel->m_activeObjectsCount + 500)) {
        for (auto it = cache.begin(); it != cache.end();) {
            if (it->first->getParent()) it++;
            else it = cache.erase(it);
        }
    }

    auto [it, inserted] = cache.try_emplace(object, object, std::move(box));

    if (!inserted) {
        it->second.transform = CachedTransform(object);
        it->second.box = std::move(box);
    }

    return it->second.box;
}

const ie::SelectionBox& ie::SelectionBox::fromObject(LevelEditorLayer* lel, GameObject* object) {
    auto& cache = getSelectionBoxCache(lel);

    auto it = cache.find(object);
    if (it != cache.end() && it->second.transform == CachedTransform(object)) return it->second.box;

    bool useTextureRect = object->m_useTextureRectForSelection ||
        (!object->m_colorSprite && !object->m_hasCustomChild && !object->m_hasAnimatedChild);

    CCSize halfSize;

    if (!object->m_hasCustomSize) {
        if (object->m_useObjectRect) {
            CCRect rect = object->getObjectRect();
            CCPoint center = rect.origin + rect.size * 0.5f;

            CCAffineTransform transform = CCAffineTransformMakeIdentity();
            transform = CCAffineTransformTranslate(transform, center.x, center.y);

            return addToCache(lel, object, { transform, rect.size * 0.5f });
        }

        halfSize = useTextureRect
            ? (object->m_obRect.size * 0.5f)
            : (object->getContentSize() * 0.5f);
    } else {
        halfSize = object->m_customSize * 0.5f;
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

    return addToCache(lel, object, { CCAffineTransformTranslate(transform, offset.x, offset.y), halfSize });
}

ie::SelectionBox ie::SelectionBox::fromRotatedRect(const CCRect& rect, const CCPoint& pivot, float rotation) {
    CCPoint center = ccp(rect.getMidX(), rect.getMidY());

    float radians = -CC_DEGREES_TO_RADIANS(rotation);
    CCPoint dist = center - pivot;

    CCPoint rotatedCenter = pivot + ccp(
        dist.x * std::cos(radians) - dist.y * std::sin(radians),
        dist.x * std::sin(radians) + dist.y * std::cos(radians)
    );

    CCAffineTransform transform = CCAffineTransformMakeIdentity();
    transform = CCAffineTransformTranslate(transform, rotatedCenter.x, rotatedCenter.y);
    transform = CCAffineTransformRotate(transform, radians);

    return { transform, rect.size * 0.5f };
}

ie::SelectionBox ie::SelectionBox::fromCorners(const std::array<CCPoint, 4>& corners) {
    CCPoint center = ranges::reduce<CCPoint>(
        corners,
        [](CCPoint& acc, CCPoint corner) {
            acc += corner;
        }
    );

    center /= 4;

    CCPoint axisX = (corners[1] - corners[0]).normalize();
    CCPoint axisY = (corners[3] - corners[0]).normalize();

    CCAffineTransform transform = {
        axisX.x, axisX.y,
        axisY.x, axisY.y,
        center.x, center.y
    };
    CCSize halfSize = CCSize(0, 0);

    for (const auto& corner : corners) {
        CCPoint dist = corner - center;

        halfSize.width = std::max(halfSize.width, std::abs(ccpDot(dist, axisX)));
        halfSize.height = std::max(halfSize.height, std::abs(ccpDot(dist, axisY)));
    }

    return { transform, halfSize };
}

bool ie::SelectionBox::containsPoint(const CCPoint& point, bool fuzzy) const {
    CCAffineTransform inverse = CCAffineTransformInvert(m_transform);

    CCPoint local = CCPointApplyAffineTransform(point, inverse);
    local = ccp(std::abs(local.x), std::abs(local.y));

    return local <= getHalfSize(fuzzy);
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

ie::SelectionBox::SelectionBox(CCAffineTransform transform, CCSize halfSize) {
    m_transform = std::move(transform);
    m_halfSize = std::move(halfSize);

    float scaleX = std::sqrt(m_transform.a * m_transform.a + m_transform.b * m_transform.b);
    float scaleY = std::sqrt(m_transform.c * m_transform.c + m_transform.d * m_transform.d);

    m_fuzzyHalfSize = CCSize(
        scaleX > 0 ? std::max(m_halfSize.width, FUZZY_RADIUS / scaleX) : m_halfSize.width,
        scaleY > 0 ? std::max(m_halfSize.height, FUZZY_RADIUS / scaleY) : m_halfSize.height
    );
}

// void ie::SelectionBox::computeFuzzySize() {
//     float scaleX = std::sqrt(m_transform.a * m_transform.a + m_transform.b * m_transform.b);
//     float scaleY = std::sqrt(m_transform.c * m_transform.c + m_transform.d * m_transform.d);

//     m_fuzzyHalfSize = CCSize(
//         scaleX > 0 ? std::max(m_halfSize.width, FUZZY_RADIUS / scaleX) : m_halfSize.width,
//         scaleY > 0 ? std::max(m_halfSize.height, FUZZY_RADIUS / scaleY) : m_halfSize.height
//     );
// }

const CCSize& ie::SelectionBox::getHalfSize(bool fuzzy) const {
    return fuzzy ? m_fuzzyHalfSize : m_halfSize;
}

std::array<CCPoint, 4> ie::SelectionBox::getCorners() const {
    const CCSize& halfSize = getHalfSize(false);

    return {
        CCPointApplyAffineTransform(halfSize * ccp(-1.f, -1.f), m_transform),
        CCPointApplyAffineTransform(halfSize * ccp(1.f, -1.f), m_transform),
        CCPointApplyAffineTransform(halfSize * ccp(1.f, 1.f), m_transform),
        CCPointApplyAffineTransform(halfSize * ccp(-1.f, 1.f), m_transform)
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
