#include "core/SettingManager.hpp"
#include "core/UpdateVisibility.hpp"
#include "events/PlaytestEvent.hpp"
#include "util/Temporary.hpp"

#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

$bind_setting(g_audioScale, "audio-scale");

class $modify(ASLevelEditorLayer, LevelEditorLayer) {
    $register_hooks("audio-scale");

    struct Fields {
        int prevAudioTrack = -1;
        int prevSongID = -1;

        ~Fields() {
            FMODAudioEngine::get()->disableMetering();
        }
    };

    $override
    void levelSettingsUpdated() {
        LevelEditorLayer::levelSettingsUpdated();

        bool hasAudioChanged = m_fields->prevAudioTrack != m_level->m_audioTrack ||
            m_fields->prevSongID != m_level->m_songID;

        m_fields->prevAudioTrack = m_level->m_audioTrack;
        m_fields->prevSongID = m_level->m_songID;

        if (!hasAudioChanged) return;

        if (m_audioEffectsLayer) {
            m_audioEffectsLayer->removeFromParent();
            m_audioEffectsLayer = nullptr;
        }

        if (m_level->m_songID == 0) {
            m_audioEffectsLayer = AudioEffectsLayer::create(LevelTools::getAudioString(m_level->m_audioTrack));
            m_objectLayer->addChild(m_audioEffectsLayer);
        }
    }

    float preUpdateAudioScale(float dt) {
        if (m_playbackMode == PlaybackMode::Not && !m_editorUI->m_isPlayingMusic) return -1.f;

        if (FMODAudioEngine::get()->m_musicVolume <= 0.f) {
            if (m_player1) m_player1->m_audioScale = 0.5f;
            if (m_player2) m_player2->m_audioScale = 0.5f;

            return 0.5f;
        }

        if (m_audioEffectsLayer) {
            ie::withFakePlayLayer([&] {
                m_audioEffectsLayer->audioStep(dt);
            });
        }

        float audioScale = m_audioEffectsLayer
            ? m_audioEffectsLayer->m_audioScale
            : FMODAudioEngine::get()->getMeteringValue();

        if (m_player1) m_player1->m_audioScale = audioScale;
        if (m_player2) m_player2->m_audioScale = audioScale;

        return audioScale;
    }

    void setAudioScale(GameObject* object, float audioScale) {
        if (!object->m_usesAudioScale || object->m_hasNoAudioScale) return;

        // orbs have their own audio scale logic in RingObject::setRScale, which only runs when
        // m_editorEnabled is false? otherwise it's the same as GameObject::setRScale. weird

        object->m_editorEnabled = false;

        if (object->m_customAudioScale) {
            float min = object->m_minAudioScale;
            float max = object->m_maxAudioScale;

            object->setRScale(min + audioScale * (max - min));
        } else {
            object->setRScale(audioScale);
        }

        object->m_editorEnabled = true;
    }

    void resetAudioScale() {
        for (const auto& object : CCArrayExt<GameObject*>(m_objects)) {
            if (auto ego = typeinfo_cast<EffectGameObject*>(object)) {
                if (ego->m_triggerEffectPlaying) {
                    ego->stopAllActions();
                    if (auto cs = ego->m_colorSprite) cs->stopAllActions();
                    if (auto gs = ego->m_glowSprite) gs->stopAllActions();

                    ego->m_triggerEffectPlaying = false;
                }
            }

            object->setRScale(1.f);
        }
    }
};

class $modify(EditorUI) {
    $register_hooks("audio-scale");

    $override
    void onPlayback(CCObject* sender) {
        EditorUI::onPlayback(sender);
        if (m_isPlayingMusic) return;

        static_cast<ASLevelEditorLayer*>(m_editorLayer)->resetAudioScale();
    }
};

$on_enable("audio-scale") {
    auto lel = static_cast<ASLevelEditorLayer*>(ctx.m_lel);

    FMODAudioEngine::get()->enableMetering();
    lel->m_fields.self(); // ensure fields dtor is called

    ctx.addEventListener(PlaytestEvent(), [lel](PlaytestMode mode) {
        if (!mode.isPlaying()) {
            lel->resetAudioScale();
        }
    });
}

float ie::preUpdateAudioScale(LevelEditorLayer* lel, float dt) {
    if (!g_audioScale) return -1.f;
    return static_cast<ASLevelEditorLayer*>(lel)->preUpdateAudioScale(dt);
}

void ie::updateAudioScale(LevelEditorLayer* lel, GameObject* object, float audioScale) {
    if (audioScale == -1.f) return;
    static_cast<ASLevelEditorLayer*>(lel)->setAudioScale(object, audioScale);
}
