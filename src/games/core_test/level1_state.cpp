#include "game.h"

void Level1State::enter() {
    mScrollY = 0;
    mPPU.setScreenMode(RetroCore::PPU::MsxPPU_BASE::ScreenMode::VSCREEN_2);
    mPPU.setBorderBackgroundColor(0);
}

void Level1State::exit() {

}

void Level1State::handleInput(retro_input_state_t input_cb) {

}

void Level1State::update(double dt) {
    mPPU.setScrollY(mScrollY++);
}

void Level1State::render() {

}
