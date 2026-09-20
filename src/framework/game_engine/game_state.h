#ifndef __RETRO_CORE_FRAMEWORK_GAME_ENGINE_GAME_STATE_H
#define __RETRO_CORE_FRAMEWORK_GAME_ENGINE_GAME_STATE_H

#include "libretro.h"

#include <functional>

namespace RetroCore {

namespace GameEngine {

class Timer {
    public:
        Timer() = default;

        Timer(float durationSeconds, std::function<void()> onExpiredAction = nullptr):Timer() {
            start(durationSeconds, onExpiredAction);
        }
        
        // Configures a timer countdown window with an optional callback action
        void start(float durationSeconds, std::function<void()> onExpiredAction = nullptr) {
            mDuration = durationSeconds;
            mTimeRemaining = durationSeconds;
            mCallback = onExpiredAction;
            mIsActive = true;
            mIsExpired = false;
        }

        // Call this inside your GameState's Update function
        void update(float dt) {
            if (!mIsActive) return;

            mTimeRemaining -= dt;
            if (mTimeRemaining <= 0.0f) {
                mTimeRemaining = 0.0f;
                mIsActive = false;
                mIsExpired = true;

                // Trigger the assigned action payload if it exists
                if (mCallback) {
                    mCallback();
                }
            }
        }

        // Needed for game mechanics like repeating weapon fire rates:
        void updateLooping(float dt) {
            if (!mIsActive) return;

            mTimeRemaining -= dt;
            if (mTimeRemaining <= 0.0f) {
                if (mCallback) mCallback();
                mTimeRemaining += mDuration; // Instantly loops without skipping sub-frame timing precision
            }
        }

        void stop() { mIsActive = false; }
        void reset() { mTimeRemaining = mDuration; mIsActive = true; mIsExpired = false; }

        bool isActive() const { return mIsActive; }
        bool hasExpired() const { return mIsExpired; }
        
        float getTimeRemaining() const { return mTimeRemaining; }
        float getTimeElapsed() const { return std::max(0.0f, mDuration - mTimeRemaining); }
        float getElapsedPercentage() const { return (mDuration > 0.0f) ? (1.0f - (mTimeRemaining / mDuration)) : 1.0f; }

    private:
        float mDuration = 0.0f;
        float mTimeRemaining = 0.0f;
        bool mIsActive = false;
        bool mIsExpired = false;
        std::function<void()> mCallback = nullptr;
};


class GameState {
    public:
        using Timer = GameEngine::Timer;
        
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
        void handleStateInput(retro_input_state_t input_cb) { if(input_cb) handleInput(input_cb); }

    protected:
        virtual void exit() = 0;
        virtual void handleInput(retro_input_state_t input_cb) = 0;
        virtual void render() = 0;
        virtual void enter() = 0;
        virtual void update(double dt) = 0;

    protected:
        double getTimeElapsed() const { return mTimeElapsed; }

        bool isAnyKeyPressed(retro_input_state_t input_cb, unsigned port) {
            // Keep track of what was held down on the previous frame
            // RETROK_LAST is usually around 320
            static std::vector<bool> prev_key_state(RETROK_LAST, false);

            bool any_new_press = false;

            for (unsigned key = RETROK_FIRST; key < RETROK_LAST; ++key) {
                // Current continuous state
                int16_t current_state = input_cb(port, RETRO_DEVICE_KEYBOARD, 0, key);
                bool is_down = (current_state != 0);

                // Check for a rising edge: Down now, but was UP last frame
                if (is_down && !prev_key_state[key]) {
                    any_new_press = true;
                }

                // Save current state for the next frame's comparison
                prev_key_state[key] = is_down;
            }
            return any_new_press;
        }

        StateManager& mStateManager;

    private:
        double   mTimeElapsed;
};

}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_GAME_STATE_H
