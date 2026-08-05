#include "freestanding.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

float getRotateCommandAngleDelta(
    EffectGameObject* trigger,
    GameObject* centerObject,
    GameObject* referenceObject,
    GameObject* targetObject,
    GameObject* minXObject,
    GameObject* minYObject,
    GameObject* maxXObject,
    GameObject* maxYObject
) {
    CCPoint centerPos = centerObject->getRealPosition();
    CCPoint targetPos = targetObject->getRealPosition();
    CCPoint referencePos = referenceObject->getRealPosition();

    float fromAngle;

    if (centerObject == referenceObject) {
        fromAngle = centerObject->getRotation();
    } else {
        CCPoint delta = centerPos - referencePos;

        if (delta.getLength() >= 0.01f) {
            float radians = delta.getAngle();
            fromAngle = 90.f - radians * (180.f / M_PI);
        } else {
            fromAngle = referenceObject->getRotation();
        }
    }

    float toAngle = 0.f;

    if (centerObject != targetObject) {
        if (!trigger->m_useMoveTarget) {
            toAngle = targetObject->getRotation();
        } else {
            CCPoint clampedTargetPos = applyPositionLimits(targetPos, minXObject, minYObject, maxXObject, maxYObject);
            CCPoint delta = centerPos - clampedTargetPos;

            if (delta.getLength() >= 0.01f) {
                float radians = delta.getAngle();
                toAngle = 90.f - radians * (180.f / M_PI);
            }
        }
    }

    float finalRotation = (toAngle - fromAngle) + trigger->m_rotationOffset;
    return GJBaseGameLayer::convertToClosestDirection(finalRotation, 180.f);
}

CCPoint applyPositionLimits(
    CCPoint pos,
    GameObject* minXObj,
    GameObject* minYObj,
    GameObject* maxXObj,
    GameObject* maxYObj
) {
    if (minXObj) pos.x = std::max(pos.x, minXObj->getRealPosition().x);
    if (minYObj) pos.y = std::max(pos.y, minYObj->getRealPosition().y);
    if (maxXObj) pos.x = std::min(pos.x, maxXObj->getRealPosition().x);
    if (maxYObj) pos.y = std::min(pos.y, maxYObj->getRealPosition().y);

    return pos;
}
