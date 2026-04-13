#pragma once

#include <Geode/binding/GameObject.hpp>

#define GAME_OBJECT(objectName, objectID) \
    constexpr int objectName = objectID; \
    inline bool is##objectName(ObjectID id) { return objectID == id; }

namespace ie::object {
    class ObjectID {
        const int m_id;

    public:
        ObjectID(int id) : m_id(id) {}
        ObjectID(GameObject* obj) : m_id(obj->m_objectID) {}

        operator int() const { return m_id; }
    };

    bool isOrb(ObjectID id);
    bool isPad(ObjectID id);
    bool isPortal(ObjectID id);
    bool isSpeedPortal(ObjectID id);

    bool isPulseRod(ObjectID id);
    bool is21Particle(ObjectID id);
    bool isParticle(ObjectID id);

    GAME_OBJECT(TallPulseRod, 15);
    GAME_OBJECT(MediumPulseRod, 16);
    GAME_OBJECT(ShortPulseRod, 17);

    GAME_OBJECT(BreakableBlock, 143);
    GAME_OBJECT(MovingFireball, 1583);

    GAME_OBJECT(SquareParticle, 1586);
    GAME_OBJECT(BubbleParticle, 1700);
    GAME_OBJECT(CustomParticle, 2065);

    GAME_OBJECT(EnableGhostTrailTrigger, 32);
    GAME_OBJECT(DisableGhostTrailTrigger, 33);
    GAME_OBJECT(EnableBGEffectTrigger, 1818);
    GAME_OBJECT(DisableBGEffectTrigger, 1819);
    GAME_OBJECT(Checkpoint, 2063);
    GAME_OBJECT(GameplayRotationTrigger, 2900);
    GAME_OBJECT(GradientTrigger, 2903);
    GAME_OBJECT(KeyframePoint, 3032);
    GAME_OBJECT(TouchToggleBlock, 3643);

    GAME_OBJECT(YellowOrb, 36);
    GAME_OBJECT(BlueOrb, 84);
    GAME_OBJECT(PinkOrb, 141);
    GAME_OBJECT(GreenOrb, 1022);
    GAME_OBJECT(BlackOrb, 1330);
    GAME_OBJECT(RedOrb, 1333);
    GAME_OBJECT(GreenDashOrb, 1704);
    GAME_OBJECT(PinkDashOrb, 1751);
    GAME_OBJECT(ToggleOrb, 1594);
    GAME_OBJECT(SpiderOrb, 3004);
    GAME_OBJECT(TeleportOrb, 3027);

    GAME_OBJECT(YellowPad, 35);
    GAME_OBJECT(BluePad, 67);
    GAME_OBJECT(PinkPad, 140);
    GAME_OBJECT(RedPad, 1332);
    GAME_OBJECT(SpiderPad, 3005);

    GAME_OBJECT(BlueGravityPortal, 10);
    GAME_OBJECT(YellowGravityPortal, 11);
    GAME_OBJECT(GreenGravityPortal, 2926);

    GAME_OBJECT(CubePortal, 12);
    GAME_OBJECT(ShipPortal, 13);
    GAME_OBJECT(BallPortal, 47);
    GAME_OBJECT(UfoPortal, 111);
    GAME_OBJECT(WavePortal, 660);
    GAME_OBJECT(RobotPortal, 745);
    GAME_OBJECT(SpiderPortal, 1331);
    GAME_OBJECT(SwingPortal, 1933);

    GAME_OBJECT(OrangeMirrorPortal, 45);
    GAME_OBJECT(BlueMirrorPortal, 46);
    GAME_OBJECT(GreenSizePortal, 99);
    GAME_OBJECT(PinkSizePortal, 101);
    GAME_OBJECT(OrangeDualPortal, 286);
    GAME_OBJECT(BlueDualPortal, 287);

    GAME_OBJECT(LinkedBlueTeleport, 747);
    GAME_OBJECT(LinkedOrangeTeleport, 749);
    GAME_OBJECT(BlueTeleportPortal, 2902);
    GAME_OBJECT(OrangeTeleportPortal, 2064);

    GAME_OBJECT(SlowSpeedPortal, 200);
    GAME_OBJECT(NormalSpeedPortal, 201);
    GAME_OBJECT(FastSpeedPortal, 202);
    GAME_OBJECT(FasterSpeedPortal, 203);
    GAME_OBJECT(FastestSpeedPortal, 1334);
}

#undef GAME_OBJECT
