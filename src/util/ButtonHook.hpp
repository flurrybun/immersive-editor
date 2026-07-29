#pragma once

namespace ie {
    using OriginalButtonCallback = geode::Function<void() const>;
    using ButtonHookCallback = geode::Function<void(cocos2d::CCObject*, OriginalButtonCallback)>;

    void hookButton(cocos2d::CCMenuItem* button, ButtonHookCallback callback);
}
