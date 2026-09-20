#include "game.h"

namespace KnightGame {

Game::Game(double target_fps): GameEngine::EngineCore<V99x8>(target_fps) {
   getPPU().createDefaultMemoryLayout();
   getPPU().clearAllSpriteAttributes();
}

[[nodiscard]] bool Game::initImpl() {
   getStateManager().pushState(std::make_unique<Boot>(getStateManager(), getPPU(), getSoundEngine(), getAssetManager()));
   return true;
}

[[nodiscard]] bool Game::shutdownImpl() {
   return true;
}

}  // namespace KnightGame