#pragma once

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
);

CCPoint applyPositionLimits(
    CCPoint pos,
    GameObject* minXObj,
    GameObject* minYObj,
    GameObject* maxXObj,
    GameObject* maxYObj
);
