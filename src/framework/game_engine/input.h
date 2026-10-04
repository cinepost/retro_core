#ifndef __RETRO_CORE_FRAMEWORK_GAME_ENGINE_INPUT_H
#define __RETRO_CORE_FRAMEWORK_GAME_ENGINE_INPUT_H

#include "libretro.h"
#include <cstdint>

namespace RetroCore {

namespace GameEngine {

constexpr int MAX_SUPPORTED_PLAYERS = 4;

class Input {
public:
    Input() {
        mCurrButtons.resize(MAX_SUPPORTED_PLAYERS, 0);
        mPrevButtons.resize(MAX_SUPPORTED_PLAYERS, 0);
    }

    void poll(retro_input_state_t input_cb) {
        if (!input_cb) return;

        // Sync old historical button arrays
        mPrevButtons = mCurrButtons;
        std::fill(mCurrButtons.begin(), mCurrButtons.end(), 0);

        const int buttonsToTrack[] = {
            RETRO_DEVICE_ID_JOYPAD_UP,    RETRO_DEVICE_ID_JOYPAD_DOWN,
            RETRO_DEVICE_ID_JOYPAD_LEFT,  RETRO_DEVICE_ID_JOYPAD_RIGHT,
            RETRO_DEVICE_ID_JOYPAD_A,     RETRO_DEVICE_ID_JOYPAD_B,
            RETRO_DEVICE_ID_JOYPAD_START, RETRO_DEVICE_ID_JOYPAD_SELECT
        };

        // Query up to 4 separate physical player ports
        for (int port = 0; port < MAX_SUPPORTED_PLAYERS; ++port) {
            uint16_t maskAccumulator = 0;
            for (int id : buttonsToTrack) {
                // Pass 'port' down directly into the Libretro callback context parameter
                if (input_cb(port, RETRO_DEVICE_JOYPAD, 0, id)) {
                    maskAccumulator |= (1 << id);
                }
            }
            mCurrButtons[port] = maskAccumulator;
        }
    }

    bool isPressed(int playerIndex, int retroJoypadId) const {
        if (playerIndex < 0 || playerIndex >= MAX_SUPPORTED_PLAYERS) return false;
        uint16_t mask = (1 << retroJoypadId);
        return ((mCurrButtons[playerIndex] & mask) != 0) && ((mPrevButtons[playerIndex] & mask) == 0);
    }

    bool isHeld(int playerIndex, int retroJoypadId) const {
        if (playerIndex < 0 || playerIndex >= MAX_SUPPORTED_PLAYERS) return false;
        uint16_t mask = (1 << retroJoypadId);
        return (mCurrButtons[playerIndex] & mask) != 0;
    }

    bool isReleased(int playerIndex, int retroJoypadId) const {
        if (playerIndex < 0 || playerIndex >= MAX_SUPPORTED_PLAYERS) return false;
        uint16_t mask = (1 << retroJoypadId);
        return ((mCurrButtons[playerIndex] & mask) == 0) && ((mPrevButtons[playerIndex] & mask) != 0);
    }

    bool isAnyKeyPressed() const noexcept {
        // Iterate through all 4 supported controller input port caches
        for (int port = 0; port < MAX_SUPPORTED_PLAYERS; ++port) {
            uint16_t current = mCurrButtons[port];
            uint16_t previous = mPrevButtons[port];

            uint16_t freshPresses = current & (~previous);

            if (freshPresses != 0) {
                return true; // A new button down event was discovered on this port!
            }
        }
        return false; // No new button transitions occurred anywhere on this frame
    }

    bool isAnyKeyPressed(unsigned int port) const noexcept {
        if (port >= MAX_SUPPORTED_PLAYERS) {
            return false;
        }

        uint16_t current = mCurrButtons[port];
        uint16_t previous = mPrevButtons[port];

        uint16_t freshPresses = current & (~previous);

        return (freshPresses != 0);
    }

private:
    std::vector<uint16_t> mCurrButtons;
    std::vector<uint16_t> mPrevButtons;
};


}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_INPUT_H