#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/GameObject.hpp>
#include <algorithm>
#include <vector>

using namespace geode::prelude;

class TrajectorySimulation {
public:
    PlayerObject* simulatedPlayer = nullptr;
    bool simulating = false;
    bool simulatedDead = false;
    float frameDt = 1.f / 240.f;

    void createPlayer(PlayLayer* layer) {
        if (!layer || simulatedPlayer) return;

        simulatedPlayer = PlayerObject::create(1, 1, layer, layer, true);
        if (simulatedPlayer) {
            simulatedPlayer->setVisible(false);
            layer->m_objectLayer->addChild(simulatedPlayer);
        }
    }

    bool wouldSurvive(PlayerObject* actual, bool press) {
        auto layer = PlayLayer::get();
        if (!layer || !actual || simulating) return false;

        createPlayer(layer);
        if (!simulatedPlayer) return false;

        simulating = true;
        simulatedDead = false;

        simulatedPlayer->copyAttributes(actual);
        simulatedPlayer->m_gravityMod = actual->m_gravityMod;
        simulatedPlayer->m_isOnGround = actual->m_isOnGround;

        if (press) simulatedPlayer->pushButton(PlayerButton::Jump);
        else simulatedPlayer->releaseButton(PlayerButton::Jump);

        for (int i = 0; i < 60; i++) {
            simulatedPlayer->m_collisionLogTop->removeAllObjects();
            simulatedPlayer->m_collisionLogBottom->removeAllObjects();
            simulatedPlayer->m_collisionLogLeft->removeAllObjects();
            simulatedPlayer->m_collisionLogRight->removeAllObjects();

            layer->checkCollisions(simulatedPlayer, frameDt, false);
            if (simulatedDead) break;
            simulatedPlayer->update(frameDt);
        }

        simulating = false;
        return !simulatedDead;
    }
};

static TrajectorySimulation g_simulation;

class $modify(TrajectoryPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects))
            return false;
        g_simulation.createPlayer(this);
        return true;
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (g_simulation.simulating &&
            player == g_simulation.simulatedPlayer &&
            object != m_anticheatSpike) {
            g_simulation.simulatedDead = true;
            return;
        }
        PlayLayer::destroyPlayer(player, object);
    }

    void onQuit() {
        g_simulation.simulatedPlayer = nullptr;
        PlayLayer::onQuit();
    }
};

class $modify(TrajectoryPlayerObject, PlayerObject) {
    struct Fields {
        struct Input {
            bool press;
            int framesLeft;
        };
        std::vector<Input> buffer;
        bool replaying = false;
    };

    bool pushButton(PlayerButton button) {
        if (g_simulation.simulating || m_fields->replaying)
            return PlayerObject::pushButton(button);

        if (button != PlayerButton::Jump)
            return PlayerObject::pushButton(button);

        m_fields->buffer.push_back({
            true,
            Mod::get()->getSettingValue<int>("buffer-frames")
        });
        return true;
    }

    bool releaseButton(PlayerButton button) {
        if (g_simulation.simulating || m_fields->replaying)
            return PlayerObject::releaseButton(button);

        if (button != PlayerButton::Jump)
            return PlayerObject::releaseButton(button);

        m_fields->buffer.push_back({
            false,
            Mod::get()->getSettingValue<int>("buffer-frames")
        });
        return true;
    }

    void update(float dt) {
        PlayerObject::update(dt);

        if (g_simulation.simulating) return;
        g_simulation.frameDt = dt;

        auto layer = PlayLayer::get();
        if (!layer || this != layer->m_player1 || m_fields->buffer.empty())
            return;

        auto& input = m_fields->buffer.front();
        bool safe = g_simulation.wouldSurvive(this, input.press);

        if (safe) {
            auto current = input;
            m_fields->buffer.erase(m_fields->buffer.begin());

            m_fields->replaying = true;
            if (current.press)
                PlayerObject::pushButton(PlayerButton::Jump);
            else
                PlayerObject::releaseButton(PlayerButton::Jump);
            m_fields->replaying = false;
        } else {
            for (auto& waiting : m_fields->buffer)
                --waiting.framesLeft;

            m_fields->buffer.erase(
                std::remove_if(
                    m_fields->buffer.begin(), m_fields->buffer.end(),
                    [](const Fields::Input& item) {
                        return item.framesLeft <= 0;
                    }
                ),
                m_fields->buffer.end()
            );
        }
    }

    void ringJump(RingObject* ring, bool force) {
        if (g_simulation
