#include "engine_core.h"
#include "game_state.h"

#include "framework/ppu/ppu_msx.h"

namespace RetroCore {

namespace GameEngine {

void StateManager::changeState(std::unique_ptr<GameState> pState) {
    assert(1 == 2 && "StateManager::changeState(..) unimplemented!!!");
}

void StateManager::pushState(std::unique_ptr<GameState> pState) {
    mStates.push_back(std::move(pState));
    mStates.back()->enterState();
}

void StateManager::popState() {
    if (!mStates.empty()) {
        mStates.back()->exitState();
        mStates.pop_back();
    }
}

void StateManager::handleInput(retro_input_state_t input_cb) {
    if (!mStates.empty()) {
        mStates.back()->handleStateInput(input_cb);
    }
}

void StateManager::update(double dt) {
    if (!mStates.empty()) {
        mStates.back()->updateState(dt);
    }
}
void StateManager::render() {
    // Render only the top state, or loop from back-to-front for transparent UI overlays
    if (!mStates.empty()) {
        mStates.back()->renderState();
    }
}

void StateManager::renderAudio(int16_t* pSamplesData, size_t samples_per_frame) {
    if(!pSamplesData) return;
    if (!mStates.empty()) {
        mStates.back()->renderStateAudio(pSamplesData, samples_per_frame);
    }
}

void StateManager::reset() {

}

void StateManager::clearAllAndChangeState(std::unique_ptr<GameState> pState) {
    while (!mStates.empty()) {
        mStates.back()->exitState();
        mStates.pop_back();
    }
    pushState(std::move(pState));
}


}  // namespace GameEngine

}  // namespace RetroCore
