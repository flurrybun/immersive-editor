#include <Geode/modify/GameObject.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

// there's a bug when creating an object's save string where if its rotation is negative it decrements by 0.01
// this happens because of an oversight in rounding the rotation to 2 decimal places
// as an example, let's see what happens when the rotation -7.08 is saved:

// -7.08 * 100.0 = -708.0
// -708.0 + 0.5 = -707.5
// (int)(-707.5) = -707  (int cast truncates towards 0)
// -707 / 100.0 = -7.07  (off by 0.01!!)

// as a result, every time you save and exit a level, all objects with negative rotation will decrement by 0.01
// i'm sure you can imagine how this is a massive issue

class $modify(GameObject) {
    $override
    gd::string getSaveString(GJBaseGameLayer* layer) {
        std::string oldStr = GameObject::getSaveString(layer);

        std::vector<std::string> tokens;
        size_t start = 0;

        for (size_t i = 0; i <= oldStr.size(); i++) {
            if (i != oldStr.size() && oldStr[i] != ',') continue;

            tokens.emplace_back(oldStr.substr(start, i - start));
            start = i + 1;
        }

        for (size_t i = 0; i + 1 < tokens.size(); i += 2) {
            const std::string& key = tokens[i];

            if (key == "6") {
                tokens[i + 1] = formatRotation(getRotation());
            } else if (key == "131") {
                tokens[i + 1] = formatRotation(getRotationX());
            } else if (key == "132") {
                tokens[i + 1] = formatRotation(getRotationY());
            }
        }

        fmt::memory_buffer buffer;

        for (size_t i = 0; i < tokens.size(); i++) {
            if (i > 0) fmt::format_to(std::back_inserter(buffer), ",");
            fmt::format_to(std::back_inserter(buffer), "{}", tokens[i]);
        }

        return gd::string(buffer.data(), buffer.size());
    }

    std::string formatRotation(float rotation) {
        // may as well round rotation to 3 decimal places instead of 2 while we're here
        return fmt::format("{}", std::round(rotation * 1000.f) / 1000.f);
    }
};
