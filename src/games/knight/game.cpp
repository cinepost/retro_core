#include "game.h"

KnightmareGame::KnightmareGame(double target_fps): GameEngine::EngineCore<V99x8>(target_fps) {
   getPPU().createDefaultMemoryLayout();
   getPPU().clearAllSpriteAttributes();
}

[[nodiscard]] bool KnightmareGame::initImpl() {
   getStateManager().pushState(std::make_unique<Boot>(getStateManager(), getPPU(), getSoundEngine(), getAssetManager()));
   return true;
}

[[nodiscard]] bool KnightmareGame::shutdownImpl() {
   return true;
}
