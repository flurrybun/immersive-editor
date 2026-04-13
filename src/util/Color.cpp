#include "Color.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

ie::HSV ie::HSV::fromRGB(const ccColor3B& rgb) {
    float r = rgb.r / 255.f;
    float g = rgb.g / 255.f;
    float b = rgb.b / 255.f;

    float cmax = std::max({ r, g, b });
    float cmin = std::min({ r, g, b });
    float delta = cmax - cmin;

    ie::HSV hsv;

    hsv.v = cmax;
    hsv.s = (cmax > 0.f) ? (delta / cmax) : 0.f;

    if (delta < 1e-6f)  hsv.h = 0.f;
    else if (cmax == r) hsv.h = (g - b) / delta;
    else if (cmax == g) hsv.h = 2.f + (b - r) / delta;
    else                hsv.h = 4.f + (r - g) / delta;

    hsv.h /= 6.f;
    if (hsv.h < 0.f) hsv.h += 1.f;

    return hsv;
}

ccColor3B ie::HSV::toRGB() const {
    if (this->s < 1e-6f) {
        unsigned char v = this->v * 255.f;
        return { v, v, v };
    }

    float h = this->h * 6.f;
    int   i = static_cast<int>(h);
    float f = h - i;
    float p = this->v * (1.f - this->s);
    float q = this->v * (1.f - this->s * f);
    float t = this->v * (1.f - this->s * (1.f - f));

    float r, g, b;

    switch (i % 6) {
        case 0:  r = this->v; g = t;       b = p;       break;
        case 1:  r = q;       g = this->v; b = p;       break;
        case 2:  r = p;       g = this->v; b = t;       break;
        case 3:  r = p;       g = q;       b = this->v; break;
        case 4:  r = t;       g = p;       b = this->v; break;
        default: r = this->v; g = p;       b = q;       break;
    }

    return {
        static_cast<unsigned char>(r * 255.f),
        static_cast<unsigned char>(g * 255.f),
        static_cast<unsigned char>(b * 255.f)
    };
}

ccColor3B ie::blendColor(const ccColor3B& first, const ccColor3B& second, float ratio) {
    return {
        static_cast<GLubyte>(first.r * (1.f - ratio) + second.r * ratio),
        static_cast<GLubyte>(first.g * (1.f - ratio) + second.g * ratio),
        static_cast<GLubyte>(first.b * (1.f - ratio) + second.b * ratio)
    };
}
