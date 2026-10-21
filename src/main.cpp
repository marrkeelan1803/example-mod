
#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        log::debug(
            "Hello from my MenuLayer::init hook! This layer has {} children.",
            this->getChildrenCount()
        );

        auto myButton = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_likeBtn_001.png"),
            this,
            menu_selector(MyMenuLayer::onMyButton)
        );

        auto menu = this->getChildByID("bottom-menu");

        if (menu) {
            menu->addChild(myButton);
            myButton->setID("my-button"_spr);
            menu->updateLayout();
        }

        return true;
    }

    void onMyButton(CCObject*) {
        FLAlertLayer::create(
            "Geode",
            "Hello from my custom mod!",
            "OK"
        )->show();
    }
};

class $modify(InputLogger, PlayerObject) {
    struct Fields {
        struct BufferedInput {
            PlayerButton button;
            bool isPress;
            int framesLeft;
        };

        std::vector<BufferedInput> buffer;
    };

    bool pushButton(PlayerButton button) {
        int maxFrames =
            Mod::get()->getSettingValue<int>("buffer-frames");

        if (maxFrames > 0) {
            m_fields->buffer.push_back({button, true, maxFrames});
            log::info("Queued press for up to {} frames", maxFrames);
            return true;
        }

        return PlayerObject::pushButton(button);
    }

    bool releaseButton(PlayerButton button) {
        int maxFrames =
            Mod::get()->getSettingValue<int>("buffer-frames");

        if (maxFrames > 0) {
            m_fields->buffer.push_back({button, false, maxFrames});
            log::info("Queued release for up to {} frames", maxFrames);
            return true;
        }

        return PlayerObject::releaseButton(button);
    }
};
