#include "ObjectIDs.hpp"

bool ie::object::isOrb(ObjectID id) {
    switch (id) {
        case YellowOrb:
        case BlueOrb:
        case PinkOrb:
        case GreenOrb:
        case BlackOrb:
        case RedOrb:
        case GreenDashOrb:
        case PinkDashOrb:
        case ToggleOrb:
        case SpiderOrb:
        case TeleportOrb:
            return true;
        default:
            return false;
    }
}

bool ie::object::isPad(ObjectID id) {
    switch (id) {
        case YellowPad:
        case BluePad:
        case PinkPad:
        case RedPad:
        case SpiderPad:
            return true;
        default:
            return false;
    }
}

bool ie::object::isPortal(ObjectID id) {
    switch (id) {
        case BlueGravityPortal:
        case YellowGravityPortal:
        case GreenGravityPortal:

        case CubePortal:
        case ShipPortal:
        case BallPortal:
        case UfoPortal:
        case WavePortal:
        case RobotPortal:
        case SpiderPortal:
        case SwingPortal:

        case OrangeMirrorPortal:
        case BlueMirrorPortal:
        case GreenSizePortal:
        case PinkSizePortal:
        case OrangeDualPortal:
        case BlueDualPortal:

        case LinkedBlueTeleport:
        case LinkedOrangeTeleport:
        case BlueTeleportPortal:
        case OrangeTeleportPortal:
            return true;
        default:
            return false;
    }
}

bool ie::object::isSpeedPortal(ObjectID id) {
    return (id >= SlowSpeedPortal && id <= FasterSpeedPortal) || id == FastestSpeedPortal;
}

bool ie::object::isPulseRod(ObjectID id) {
    return id >= TallPulseRod && id <= ShortPulseRod;
}

bool ie::object::is21Particle(ObjectID id) {
    return id == SquareParticle || id == BubbleParticle;
}

bool ie::object::isParticle(ObjectID id) {
    return is21Particle(id) || id == CustomParticle;
}
