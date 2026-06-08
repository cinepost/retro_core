#ifndef __RETRO_CORE_FRAMEWORK_GAME_ENGINE_GAME_STATE_H
#define __RETRO_CORE_FRAMEWORK_GAME_ENGINE_GAME_STATE_H

#include "libretro.h"


namespace RetroCore {

namespace GameEngine {

class GameState {
    public:
        GameState(StateManager& sm) : mStateManager(sm), mTimeElapsed(0.0) {}
        virtual ~GameState() = default;

        void updateState(double dt) {
            mTimeElapsed += dt;
            update(dt);
        }

        void enterState() {
            mTimeElapsed = 0;
            enter();
        }

        void exitState() { exit(); }
        void renderState() { render(); }
        void renderStateAudio(int16_t* pSamplesData, size_t samples_per_frame) { renderAudio(pSamplesData, samples_per_frame); }
        void handleStateInput(retro_input_state_t cb) { handleInput(cb); }

    protected:
        virtual void exit() = 0;
        virtual void handleInput(retro_input_state_t cb) = 0;
        virtual void render() = 0;
        virtual void renderAudio(int16_t* pSamplesData, size_t samples_per_frame) = 0;
        virtual void enter() = 0;
        virtual void update(double dt) = 0;

    protected:
        double getTimeElapsed() const { return mTimeElapsed; }
        StateManager& mStateManager;

    private:
        double   mTimeElapsed;
};

/*
// UI & Sequence States
class IntroState : public GameState {
    public:
        using GameState::GameState;
        void enter() override;
        void exit() override;
        void handleInput() override;
        void update(double dt) override; // Tracks timer to automatically transition
        void render() override; // Draws splash art
};

class CutsceneState : public GameState {
    public:
        CutsceneState(StateManager& sm, const std::string& scriptPath);
        void enter() override;
        void exit() override;
        void handleInput() override; // Allows skipping via buttons
        void update(double dt) override; // Advances dialogue/animation frames
        void render() override;
};

class CreditsState : public GameState {
    public:
        using GameState::GameState;
        void enter() override;
        void exit() override;
        void handleInput() override;
        void update(double dt) override; // Scrolls text upwards vertically
        void render() override;
};

// Active Gameplay State
class LevelState : public GameState {
    public:
        LevelState(StateManager& sm, const std::string& levelMapPath);
        void enter() override;
        void exit() override;
        void handleInput() override;
        void update(double dt) override; // System loops over game objects
        void render() override; // Camera view handling (scrolling)

    private:
        void checkCollisions();
        void managePlayerProgression(); // Handles total lives, scores, level switches
        
        struct Vector2D { float x; float y; } m_cameraOffset;
        int m_playerScore = 0;
        int m_playerLives = 3;
        
        std::vector<std::unique_ptr<class GameObject>> m_entities;
};
*/

}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_GAME_STATE_H
