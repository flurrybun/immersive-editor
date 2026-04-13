#pragma once

#include <Geode/utils/function.hpp>

namespace ie {
    struct Override {
        geode::Function<void() const> m_apply;
        geode::Function<void() const> m_restore;

        template <typename T>
        Override(T* ptr, T value) {
            m_apply = [ptr, value] { *ptr = value; };
            m_restore = [ptr, prev = *ptr] { *ptr = prev; };
        }

        template <typename T>
        Override(T** ptr, std::nullptr_t) : Override(ptr, (T*)nullptr) {}
    };

    void withTemporary(std::initializer_list<Override> overrides, geode::Function<void()> function);

    void withFakePlayLayer(geode::Function<void()> function);
    void withFakePlayLayer(geode::Function<void(PlayLayer* pl)> function);
}
