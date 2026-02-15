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
        std::vector<std::string> newTokens;
        size_t start = 0;

        for (size_t i = 0; i <= oldStr.size(); i++) {
            if (i != oldStr.size() && oldStr[i] != ',') continue;

            tokens.emplace_back(oldStr.substr(start, i - start));
            start = i + 1;
        }

        // if rotation x and y are the same, they're combined into a single rotation property (key 6)
        // however since this happens after gd's broken rounding, there are cases where gd sees the rotations
        // as different but we round them to the same value, e.g. rotation x = -36 and y = -35.999996
        // so gd rounds to (-36, -35.99) but we round to (-36, -36)

        // when this happens, rotation x and y get set to the same value, and so upon loading the object
        // gd uses the single rotation property, which is zero

        float rotationX = roundToThousandth(getRotationX());
        float rotationY = roundToThousandth(getRotationY());
        bool addedNewTokens = false;

        for (size_t i = 0; i + 1 < tokens.size(); i += 2) {
            const std::string& key = tokens[i];

            // i don't think it matters where in the save string the properties are, i just think it'd look
            // ugly if the rotation properties were at the very end for no reason

            if (key == "6" || key == "131" || key == "132") {
                if (addedNewTokens) continue;

                if (rotationX == rotationY) {
                    newTokens.push_back("6");
                    newTokens.push_back(fmt::to_string(rotationX));
                } else {
                    newTokens.push_back("131");
                    newTokens.push_back(fmt::to_string(rotationX));
                    newTokens.push_back("132");
                    newTokens.push_back(fmt::to_string(rotationY));
                }

                addedNewTokens = true;
                continue;
            }

            newTokens.push_back(tokens[i]);
            newTokens.push_back(tokens[i + 1]);
        }

        fmt::memory_buffer buffer;
        for (size_t i = 0; i < newTokens.size(); i++) {
            if (i == 0) fmt::format_to(std::back_inserter(buffer), "{}", newTokens[i]);
            else fmt::format_to(std::back_inserter(buffer), ",{}", newTokens[i]);
        }

        return gd::string(buffer.data(), buffer.size());
    }

    float roundToThousandth(float value) {
        return std::round(value * 1000.f) / 1000.f;
    }
};
