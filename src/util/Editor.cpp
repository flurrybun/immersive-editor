#include "Editor.hpp"

#include <Geode/modify/LevelEditorLayer.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

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
