#include "Temporary.hpp"

void ie::withTemporary(std::initializer_list<Override> overrides, geode::Function<void()> function) {
    for (const auto& override : overrides) {
        override.m_apply();
    }

    function();

    for (const auto& override : overrides) {
        override.m_restore();
    }
}

void ie::withFakePlayLayer(geode::Function<void()> function) {
    GameManager* gm = GameManager::get();
    PlayLayer* prev = gm->m_playLayer;

    if (prev) {
        function();
        return;
    }

    gm->m_playLayer = reinterpret_cast<PlayLayer*>(gm->m_gameLayer);
    function();
    gm->m_playLayer = prev;
}

void ie::withFakePlayLayer(geode::Function<void(PlayLayer* pl)> function) {
    GameManager* gm = GameManager::get();
    PlayLayer* prev = gm->m_playLayer;

    if (prev) {
        function(prev);
        return;
    }

    gm->m_playLayer = reinterpret_cast<PlayLayer*>(gm->m_gameLayer);
    function(gm->m_playLayer);
    gm->m_playLayer = prev;
}
