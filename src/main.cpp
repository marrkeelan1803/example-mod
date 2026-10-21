#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/GameObject.hpp>

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

        if (press)
            simulatedPlayer->pushButton(PlayerButton::Jump);
        else
            simulatedPlayer->releaseButton(PlayerButton::Jump);

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

        if (!g_simulation.simulating)
            g_simulation.frameDt = dt;
    }

    void ringJump(RingObject* ring, bool force) {
        if (g_simulation.simulating) return;
        PlayerObject::ringJump(ring, force);
    }

    void incrementJumps() {
        if (g_simulation.simulating) return;
        PlayerObject::incrementJumps();
    }

    void playSpiderDashEffect(cocos2d::CCPoint from, cocos2d::CCPoint to) {
        if (g_simulation.simulating) return;
        PlayerObject::playSpiderDashEffect(from, to);
    }
};

class $modify(TrajectoryGameLayer, GJBaseGameLayer) {
    bool canBeActivatedByPlayer(PlayerObject* player, EffectGameObject* object) {
        if (g_simulation.simulating) return false;
        return GJBaseGameLayer::canBeActivatedByPlayer(player, object);
    }

    void flipGravity(PlayerObject* player, bool flip, bool instant) {
        if (g_simulation.simulating) return;
        GJBaseGameLayer::flipGravity(player, flip, instant);
    }

    void playerTouchedRing(PlayerObject* player, RingObject* ring) {
        if (g_simulation.simulating) return;
        GJBaseGameLayer::playerTouchedRing(player, ring);
    }

    void collisionCheckObjects(PlayerObject* player, gd::vector<GameObject*>* objects,
                               int count, float dt) {
        if (g_simulation.simulating) {
            gd::vector<GameObject*> solidsOnly;
            for (int i = 0; i < count; i++) {
                auto object = objects->at(i);
                if (object->m_objectType == GameObjectType::Solid ||
                    object->m_objectType == GameObjectType::Hazard ||
                    object->m_objectType == GameObjectType::AnimatedHazard ||
                    object->m_objectType == GameObjectType::Slope) {
                    solidsOnly.push_back(object);
                }
            }
            GJBaseGameLayer::collisionCheckObjects(
                player, &solidsOnly, static_cast<int>(solidsOnly.size()), dt
            );
            return;
        }

        GJBaseGameLayer::collisionCheckObjects(player, objects, count, dt);
    }

    void updateCamera(float dt) {
        GJBaseGameLayer::updateCamera(dt);
    }
};

class $modify(TrajectoryEffectObject, EffectGameObject) {
    void triggerObject(GJBaseGameLayer* layer, int id, gd::vector<int> const* values) {
        if (g_simulation.simulating) return;
        EffectGameObject::triggerObject(layer, id, values);
    }
};

class $modify(TrajectoryGameObject, GameObject) {
    void playShineEffect() {
        if (g_simulation.simulating) return;
        GameObject::playShineEffect();
    }
};

class $modify(TrajectoryHardStreak, HardStreak) {
    void addPoint(cocos2d::CCPoint point) {
        if (g_simulation.simulating) return;
        HardStreak::addPoint(point);
    }
};
