#pragma once

#include <Geode/cocos/include/ccTypes.h>

namespace ie {
    struct HSV {
        float h, s, v;

        HSV() : h(0.f), s(0.f), v(0.f) {}
        HSV(float h, float s, float v) : h(h), s(s), v(v) {}

        static HSV fromRGB(const cocos2d::ccColor3B& rgb);
        cocos2d::ccColor3B toRGB() const;
    };

    cocos2d::ccColor3B blendColor(const cocos2d::ccColor3B& first, const cocos2d::ccColor3B& second, float ratio);
}
