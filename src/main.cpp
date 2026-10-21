
#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

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

        m_fields->buffer.push_back({button, true, maxFrames});

        log::info("Queued press");
        return true;
    }

    bool releaseButton(PlayerButton button) {
        int maxFrames =
            Mod::get()->getSettingValue<int>("buffer-frames");

        m_fields->buffer.push_back({button, false, maxFrames});

        log::info("Queued release");
        return true;
    }

    void update(float dt) {
        PlayerObject::update(dt);

        auto& buffer = m_fields->buffer;

        if (buffer.empty()) {
            return;
        }

        // Process the oldest input.
        auto input = buffer.front();
        buffer.erase(buffer.begin());

        if (input.isPress) {
            PlayerObject::pushButton(input.button);
        } else {
            PlayerObject::releaseButton(input.button);
        }

        // Age inputs that are still waiting.
        for (auto& waiting : buffer) {
            waiting.framesLeft--;
        }

        // Remove inputs that have expired.
        buffer.erase(
            std::remove_if(
                buffer.begin(),
                buffer.end(),
                [](const Fields::BufferedInput& input) {
                    return input.framesLeft <= 0;
                }
            ),
            buffer.end()
        );
    }
};
