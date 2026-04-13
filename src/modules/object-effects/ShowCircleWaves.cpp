#include "core/SettingManager.hpp"
#include "util/Editor.hpp"
#include "util/Temporary.hpp"
#include "util/ObjectIDs.hpp"

#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/RingObject.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

// almost every function spawning a circle wave had to be decompiled because most of
// them get added to m_circleWaveArray, a PlayLayer member, causing undefined behavior.
// i could pretend to be upset but honestly it was quite fun and good practice using ghidra

// rob's internal names can be very arcane so i annotated what each hook does for future reference

class $modify(PlayerObject) {
    $register_hooks("show-circle-waves");

    $override
    void spawnPortalCircle(ccColor3B color, float startRadius) {
        // ⏺️ circle wave on activating portal

        if (!ie::inEditor()) {
            PlayerObject::spawnPortalCircle(color, startRadius);
            return;
        }

        if (m_maybeReducedEffects || GameManager::get()->m_performanceMode || !m_lastActivatedPortal) return;
        if (m_lastPortalPos.x == 0 && m_lastPortalPos.y == 0) return;

        CCCircleWave* circleWave = CCCircleWave::create(startRadius, 5.f, 0.3f, true, true);

        circleWave->m_color = color;
        circleWave->setPosition(m_lastPortalPos);
        circleWave->followObject(m_lastActivatedPortal, true);
        m_parentLayer->addChild(circleWave);
    }

    $override
    void toggleDartMode(bool p0, bool p1) {
        // ⏺️ circle wave on activating wave portal

        if (!ie::inEditor()) {
            PlayerObject::toggleDartMode(p0, p1);
            return;
        }

        ie::withFakePlayLayer([&] {
            ie::withTemporary({
                { &m_playEffects, true },
            }, [&] {
                PlayerObject::toggleDartMode(p0, p1);
            });
        });
    }

    $override
    void togglePlayerScale(bool p0, bool p1) {
        // ⏺️ size portal lightning
        // ⏺️ background flashes
        // 🔀 call PlayerObject::spawnScaleCircle

        if (!ie::inEditor()) {
            PlayerObject::togglePlayerScale(p0, p1);
            return;
        }

        ie::withFakePlayLayer([&] {
            ie::withTemporary({
                { &m_playEffects, true },
            }, [&] {
                PlayerObject::togglePlayerScale(p0, p1);
            });
        });
    }

    $override
    void spawnScaleCircle() {
        // ⏺️ circle wave on activating size portal

        if (!ie::inEditor()) {
            PlayerObject::spawnScaleCircle();
            return;
        }

        if (GameManager::get()->m_performanceMode || GJBaseGameLayer::get()->m_skipArtReload) return;

        bool isBig = m_vehicleSize == 1.f;

        CCCircleWave* circleWave = CCCircleWave::create(
            isBig ? 10.f : 50.f,
            isBig ? 40.f : 2.f,
            isBig ? 0.3f : 0.25f,
            isBig,
            true
        );

        circleWave->m_color = isBig ? ccc3(0, 255, 150) : ccc3(255, 0, 150);
        circleWave->followObject(this, true);
        m_parentLayer->addChild(circleWave);
    }

    $override
    void ringJump(RingObject* ring, bool p1) {
        // ⏺️ circle wave on activating orb

        if (!ie::inEditor()) {
            PlayerObject::ringJump(ring, p1);
            return;
        }

        if (!m_stateRingJump2 || m_isDashing || !m_stateJumpBuffered) return;

        GameObjectType type = ring->getType();
        bool isCustomRing = type == GameObjectType::CustomRing;
        bool isTeleportOrb = type == GameObjectType::TeleportOrb;
        bool isSpecialRing = isCustomRing || isTeleportOrb;

        bool touchedAnyRing = m_touchedRing || isSpecialRing;
        bool touchedNonCustomRing = m_touchedCustomRing || !isCustomRing;

        if (touchedAnyRing && touchedNonCustomRing) {
            if (m_touchedGravityPortal) return;
            if (!isTeleportOrb) return;
        }

        PlayerObject::ringJump(ring, p1);
        ring->playTriggerEffect();

        if (ring->m_hasNoEffects) return;

        CCCircleWave* circleWave = CCCircleWave::create(
            ring->getType() == GameObjectType::RedJumpRing ? 42.f : 35.f,
            5.f, 0.35f, true, true
        );

        circleWave->m_color = getCircleWaveColor(ring);
        circleWave->followObject(ring, true);
        m_parentLayer->addChild(circleWave);
    }

    $override
    void playBumpEffect(int objectType, GameObject* player) {
        // ⏺️ circle wave on activating pad

        if (!ie::inEditor()) {
            PlayerObject::playBumpEffect(objectType, player);
            return;
        }

        if (GameManager::get()->m_performanceMode || !m_lastActivatedPortal) return;

        CCCircleWave* circleWave = CCCircleWave::create(
            m_vehicleSize >= 1.f && objectType == static_cast<int>(GameObjectType::RedJumpPad) ? 12.f : 10.f,
            40.f, 0.25f, false, true
        );

        // why do pads use m_lastActivatedPortal but orbs just use the orb's position

        circleWave->m_color = getCircleWaveColor(m_lastActivatedPortal);
        circleWave->setPosition(m_lastPortalPos);
        circleWave->followObject(m_lastActivatedPortal, true);
        m_parentLayer->addChild(circleWave);
    }

    $override
    void spawnDualCircle() {
        // ⏺️ circle wave on entering dual mode

        if (!ie::inEditor()) {
            PlayerObject::spawnDualCircle();
            return;
        }

        if (GameManager::get()->m_performanceMode || GJBaseGameLayer::get()->m_skipArtReload) return;

        CCCircleWave* circleWave = CCCircleWave::create(50.f, 2.f, 0.25f, true, true);

        circleWave->m_color = m_playerColor1;
        circleWave->followObject(this, true);
        m_parentLayer->addChild(circleWave);
    }

    $override
    void playSpiderDashEffect(CCPoint from, CCPoint to) {
        // ⏺️ various effects on spider teleport or activating spider orb/pad

        if (!ie::inEditor()) {
            PlayerObject::playSpiderDashEffect(from, to);
            return;
        }

        if (m_isHidden || GameManager::get()->m_performanceMode) return;

        float directionMod = m_isGoingLeft ? -1.f : 1.f;
        from.x += directionMod * 7.5f;
        to.x += (directionMod * 7.5f) + 6.f;

        ccColor3B color = m_swapColors ? m_playerColor1 : m_playerColor2;

        CCCircleWave* fromCircle = CCCircleWave::create(m_vehicleSize * 13.f, 1.f, 0.15f, false, false);
        fromCircle->m_color = color;
        fromCircle->setPosition(from);
        m_parentLayer->addChild(fromCircle);

        CCCircleWave* toCircle = CCCircleWave::create(m_vehicleSize * 26.f, 2.f, 0.25f, false, false);
        toCircle->m_color = color;
        toCircle->setPosition(to);
        m_parentLayer->addChild(toCircle);

        CCCircleWave* toCircle2 = CCCircleWave::create(m_vehicleSize * 10.f, m_vehicleSize * 45.f, 0.25f, false, false);
        toCircle2->m_color = color;
        toCircle2->m_circleMode = CircleMode::Outline;
        toCircle2->setPosition(to);
        m_parentLayer->addChild(toCircle2);

        flashPlayer(0.3f, 0.05f, {255, 255, 255}, {255, 255, 255});

        m_spiderSprite->stopActionByTag(10);
        m_spiderSprite->setScale(0.4);

        m_spiderSprite->runAction(
            CCEaseElasticOut::create(
                CCScaleTo::create(0.45f, 1.f),
                0.35f
            )
        );

        CCSprite* dash = CCSprite::createWithSpriteFrameName("spiderDash_002.png");
        m_parentLayer->addChild(dash, 40);

        CCPoint midPoint = m_isSideways
            ? ccp((from.x + to.x) * 0.5f, from.y)
            : ccp(from.x, (from.y + to.y) * 0.5f);

        dash->setPosition(midPoint);
        dash->setRotation(m_isSideways ? 0.f : 90.f);
        dash->setScaleX((ccpDistance(from, to) + 30.f) / dash->getContentWidth());

        float rotation = m_isSideways ? 0.f : 90.f;
        if (m_isSideways ? to.x > from.x : from.y > to.y) rotation += 180.f;
        dash->setRotation(rotation);

        dash->setColor(color);
        dash->setBlendFunc({ GL_SRC_ALPHA, GL_ONE });

        CCArray* spriteFrames = CCArray::createWithCapacity(8);

        for (size_t i = 1; i <= 8; i++) {
            std::string frameName = fmt::format("spiderDash_00{}.png", i);
            CCSpriteFrame* frame = CCSpriteFrameCache::get()->spriteFrameByName(frameName.c_str());

            spriteFrames->addObject(frame);
        }

        CCAnimation* dashAnimation = CCAnimation::createWithSpriteFrames(spriteFrames, 0.04f);

        dash->runAction(CCSequence::create(
            CCAnimate::create(dashAnimation),
            CallFuncExt::create([dash] {
                dash->removeFromParent();
            }),
            nullptr
        ));

        if (!m_isSpider || !m_isPlatformer) {
            m_playerGroundParticles->resetSystem();
        }
    }

    $override
    void spawnCircle() {
        // ⏺️ circle wave on respawning

        if (!ie::inEditor()) {
            PlayerObject::spawnCircle();
            return;
        }

        if (GameManager::get()->m_performanceMode) return;

        CCCircleWave* circleWave = CCCircleWave::create(70.f, 2.f, 0.3f, true, true);

        circleWave->m_circleMode = CircleMode::Outline;
        circleWave->m_color = m_playerColor1;
        circleWave->followObject(this, true);
        m_parentLayer->addChild(circleWave);
    }

    ccColor3B getCircleWaveColor(GameObject* object) {
        switch (object->m_objectID) {
            case ie::object::YellowOrb:
            case ie::object::YellowPad:

            // guess rob forgot to change the color of these :P

            case ie::object::RedPad:
            case ie::object::TeleportOrb:
                return ccc3(255, 200, 0);
            case ie::object::BlueOrb:
            case ie::object::BluePad:
                return ccc3(0, 255, 255);
            case ie::object::PinkOrb:
            case ie::object::PinkPad:
                return ccc3(255, 0, 255);
            case ie::object::RedOrb:
                return ccc3(255, 100, 0);
            case ie::object::BlackOrb:
                return m_gameLayer->m_effectManager->activeColorForIndex(1007);
            case ie::object::ToggleOrb:
                if (object->m_colorSprite) {
                    return object->m_colorSprite->getColor();
                } else {
                    return ccc3(0, 0, 0);
                }
            case ie::object::GreenOrb:
            case ie::object::GreenDashOrb:
                return ccc3(0, 255, 0);
            case ie::object::PinkDashOrb:
                return ccc3(255, 0, 255);
            case ie::object::SpiderOrb:
            case ie::object::SpiderPad:
                return ccc3(125, 0, 255);
            default:
                return ccc3(0, 0, 0);
        }
    }
};

class $modify(GJBaseGameLayer) {
    $register_hooks("show-circle-waves");

    $override
    void toggleDualMode(GameObject* p0, bool p1, PlayerObject* p2, bool p3) {
        // 🔀 call PlayerObject::spawnDualCircle

        ie::withTemporary({
            { &m_isEditor, false },
        }, [&] {
            GJBaseGameLayer::toggleDualMode(p0, p1, p2, p3);
        });
    }

    $override
    void checkRepellPlayer() {
        // ⏺️ circle wave on dual balls repelling

        ie::withTemporary({
            { &m_isEditor, false },
        }, [&] {
            GJBaseGameLayer::checkRepellPlayer();
        });
    }
};

class $modify(LevelEditorLayer) {
    $register_hooks("show-circle-waves");

    $override
    void onPlaytest() {
        // ⏺️ show respawn effect on playtesting from start position

        LevelEditorLayer::onPlaytest();
        if (!m_startPosObject) return;

        m_player1->playSpawnEffect();
        if (m_gameState.m_isDualMode) m_player2->playSpawnEffect();
    }
};

class $modify(GameObject) {
    $register_hooks("show-circle-waves");

    $override
    void playShineEffect() {
        // ⏺️ white flash on activating portal
        // ⏺️ circle wave on activating speed portal

        if (!ie::inEditor()) {
            GameObject::playShineEffect();
            return;
        }

        if (!ie::object::isSpeedPortal(this)) {
            ie::withFakePlayLayer([&] {
                ie::withTemporary({
                    { &m_editorEnabled, false },
                }, [&] {
                    GameObject::playShineEffect();
                });
            });

            return;
        }

        if (m_hasNoEffects || getOpacity() == 0 || !getParent()) return;

        auto shine = CCSpritePlus::createWithSpriteFrameName(getSpeedShineFrameName());
        GJBaseGameLayer::get()->m_objectLayer->addChild(shine);

        shine->setRotation(getRotation());
        shine->setPosition(getRealPosition());
        shine->setScaleX(getScaleX());
        shine->setScaleY(getScaleY());

        shine->setOpacity(0);
        shine->setBlendFunc({ GL_SRC_ALPHA, GL_ONE });
        shine->followSprite(this);

        shine->runAction(CCSequence::create(
            CCFadeIn::create(0.05f),
            CCFadeOut::create(0.4f),
            CallFuncExt::create([shine] {
                shine->stopFollow();
                shine->removeFromParent();
            }),
            nullptr
        ));

        float endRadius = 60.f;

        switch (m_objectID) {
            case ie::object::NormalSpeedPortal: endRadius = 65.f; break;
            case ie::object::FastSpeedPortal: endRadius = 70.f; break;
            case ie::object::FasterSpeedPortal: endRadius = 80.f; break;
            case ie::object::FastestSpeedPortal: endRadius = 90.f; break;
        }

        CCCircleWave* circleWave = CCCircleWave::create(
            5.f, endRadius, 0.3f, false, true
        );

        circleWave->m_circleMode = CircleMode::Outline;
        circleWave->m_color = getSpeedCircleWaveColor();
        circleWave->followObject(this, true);
        GJBaseGameLayer::get()->m_objectLayer->addChild(circleWave);
    }

    const char* getSpeedShineFrameName() {
        switch (m_objectID) {
            case ie::object::SlowSpeedPortal:
                return "boost_01_shine_001.png";
            case ie::object::NormalSpeedPortal:
                return "boost_02_shine_001.png";
            case ie::object::FastSpeedPortal:
                return "boost_03_shine_001.png";
            case ie::object::FasterSpeedPortal:
                return "boost_04_shine_001.png";
            case ie::object::FastestSpeedPortal:
                return "boost_05_shine_001.png";
            default:
                return "diffIcon_02_btn_001.png";
        }
    }

    ccColor3B getSpeedCircleWaveColor() {
        switch (m_objectID) {
            case ie::object::SlowSpeedPortal:
                return ccc3(255, 255, 0);
            case ie::object::NormalSpeedPortal:
                return ccc3(0, 150, 255);
            case ie::object::FastSpeedPortal:
                return ccc3(0, 255, 150);
            case ie::object::FasterSpeedPortal:
                return ccc3(255, 0, 255);
            case ie::object::FastestSpeedPortal:
                return ccc3(255, 50, 50);
            default:
                return ccc3(0, 0, 0);
        }
    }
};

class $modify(RingObject) {
    $register_hooks("show-circle-waves");

    $override
    void powerOnObject(int state) {
        // ⏺️ circle wave on hovering over orb

        if (!ie::inEditor()) {
            RingObject::powerOnObject(state);
            return;
        }

        m_poweredOn = true;
        m_state = state;

        if (m_isRingPoweredOn) return;
        m_isRingPoweredOn = true;

        if (
            ie::object::isTouchToggleBlock(this) ||
            m_hasNoEffects ||
            GameManager::get()->m_performanceMode
        ) return;

        CCCircleWave* circleWave = CCCircleWave::create(5.f, 55.f, 0.25f, false, true);

        circleWave->m_circleMode = CircleMode::Outline;
        circleWave->followObject(this, false);
        GJBaseGameLayer::get()->m_objectLayer->addChild(circleWave);
    }
};
