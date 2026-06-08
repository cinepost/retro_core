#include "game.h"

[[nodiscard]] bool TestMsxGame::initImpl() {
   getStateManager().pushState(std::make_unique<BootState>(mPPU, getStateManager()));
   return true;
}

[[nodiscard]] bool TestMsxGame::shutdownImpl() {
   return true;
}
