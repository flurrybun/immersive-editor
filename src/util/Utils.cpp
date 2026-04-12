#include "Utils.hpp"

#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

bool s_inEditor = false;

class $modify(LevelEditorLayer) {
    struct Fields {
        ~Fields() {
            s_inEditor = false;
        }
    };

    static void onModify(auto& self) {
        (void)self.setHookPriority("LevelEditorLayer::init", Priority::FirstPre);
    }

    bool init(GJGameLevel* p0, bool p1) {
        s_inEditor = true;
        m_fields.self();

        return LevelEditorLayer::init(p0, p1);
    }
};

// class $modify(PlayLayer) {
//     $override
//     bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
//         if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

//         s_inEditor = false;

//         return true;
//     }
// };

bool ie::isAmazon() {
#ifdef GEODE_IS_ANDROID
    return ((GJMoreGamesLayer* volatile)nullptr)->getMoreGamesList()->count() == 0;
#else
    return false;
#endif
}

bool ie::inEditor() {
    return s_inEditor;
}

bool ie::isEditorTopLevel(LevelEditorLayer* lel) {
    // stolen from custom keybinds

    if (CCIMEDispatcher::sharedDispatcher()->hasDelegate()) return false;

    auto handler = static_cast<CCKeyboardHandler*>(CCKeyboardDispatcher::get()->m_pDelegates->lastObject());
    if (!handler) return false;

    return static_cast<CCKeyboardDelegate*>(lel->m_editorUI) == handler->m_pDelegate;
}

bool ie::isObjectLayerVisible(GameObject* object, LevelEditorLayer* lel) {
    if (lel->m_currentLayer == -1 || lel->m_playbackMode == PlaybackMode::Playing) return true;

    if (object->m_editorLayer2 != 0 && object->m_editorLayer2 == lel->m_currentLayer) return true;
    return object->m_editorLayer == lel->m_currentLayer;
}

bool ie::isLinkControlsEnabled(LevelEditorLayer* lel) {
    return !lel->m_editorUI->m_linkControlsDisabled && lel->m_editorUI->m_stickyControlsEnabled;
}

ie::HSV ie::rgbToHsv(const ccColor3B& color) {
    float r = color.r / 255.f, g = color.g / 255.f, b = color.b / 255.f;
    float cmax = std::max({r, g, b}), cmin = std::min({r, g, b});
    float delta = cmax - cmin;

    ie::HSV hsv = { 0.f, 0.f, cmax };
    if (delta < 1e-6f) return hsv;

    hsv.s = delta / cmax;

    if (cmax == r) hsv.h = 60.f * std::fmod((g - b) / delta, 6.f);
    else if (cmax == g) hsv.h = 60.f * ((b - r) / delta + 2.f);
    else hsv.h = 60.f * ((r - g) / delta + 4.f);

    if (hsv.h < 0.f) hsv.h += 360.f;
    return hsv;
}

ccColor3B ie::blendColor(const ccColor3B& first, const ccColor3B& second, float ratio) {
    return {
        static_cast<GLubyte>(first.r * (1.f - ratio) + second.r * ratio),
        static_cast<GLubyte>(first.g * (1.f - ratio) + second.g * ratio),
        static_cast<GLubyte>(first.b * (1.f - ratio) + second.b * ratio)
    };
}
