#include "Utils.hpp"

bool ie::isObjectLayerVisible(GameObject* object, LevelEditorLayer* editor) {
    if (editor->m_currentLayer == -1 || editor->m_playbackMode == PlaybackMode::Playing) return true;

    if (object->m_editorLayer2 != 0 && object->m_editorLayer2 == editor->m_currentLayer) return true;
    return object->m_editorLayer == editor->m_currentLayer;
}
