#include "game.h"

TestMsxGame::TestMsxGame(double target_fps): GameEngine::EngineCore<V99x8>(target_fps) {
   getPPU().createDefaultMemoryLayout();
   getPPU().clearAllSpriteAttributes();
}

[[nodiscard]] bool TestMsxGame::initImpl() {
   getStateManager().pushState(std::make_unique<BootState>(getPPU(), getStateManager()));
   return true;
}

[[nodiscard]] bool TestMsxGame::shutdownImpl() {
   return true;
}
