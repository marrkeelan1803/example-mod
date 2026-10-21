#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

class TrajectorySimulation {
public:
    PlayerObject* simulatedPlayer = nullptr;
    bool simulating = false;
    bool simulatedDead = false;
    float frameDt = 1.f / 240.f;

    void createPlayer(PlayLayer* layer) {
        if (!layer || simulatedPlayer) return;

        simulatedPlayer = PlayerObject::create(
            1, 1, layer, layer, true
        );

        if (simulatedPlayer) {
            simulatedPlayer->setVisible(false);
            layer->m_objectLayer->addChild(simulatedPlayer);
        }
    }

    bool wouldSurvive(PlayerObject* actual, bool press) {
        auto layer = PlayLayer::get();
        if (!layer || !actual) return false;

        createPlayer(layer);
        if (!simulatedPlayer) return false;

        simulating = true;
        simulatedDead = false;

        simulatedPlayer->copyAttributes(actual);
        simulatedPlayer->m_gravityMod = actual->m_gravityMod;
        simulatedPlayer->m_isOnGround = actual->m_isOnGround;

        if (press) {
            simulatedPlayer->pushButton(PlayerButton::Jump);
        } else {
            simulatedPlayer->releaseButton(PlayerButton::Jump);
        }

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
    void update(float dt) {
        PlayerObject::update(dt);

        if (!g_simulation.simulating) {
            g_simulation.frameDt = dt;
        }
    }
};

class $modify(TrajectoryGameLayer, GJBaseGameLayer) {
    void updateCamera(float dt) {
        auto layer = PlayLayer::get();

        if (layer && layer->m_player1 &&
            g_simulation.simulatedPlayer) {
            g_simulation.wouldSurvive(layer->m_player1, true);
        }

        GJBaseGameLayer::updateCamera(dt);
    }
};
