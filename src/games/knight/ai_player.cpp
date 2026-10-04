#include "ai_player.h"

namespace KnightGame {

void SimpleAIPlayer::update(const Player& player, const WorldContext& ctx, AiGamepad& gamepad) {
    uint16_t retro_joypad_mask = 0;

    // Priority 1: Check for incoming immediate threats (Bullets/Enemies right above)
    WorldContext::EntityData closest_threat;
    if (findImmediateThreat(player, ctx, closest_threat)) {
        // Dodge horizontally away from the threat
        if (closest_threat.x > player.getPosX()) {
            gamepad.pressLeft();
        } else {
            gamepad.pressRight();
        }
    } 
    // Priority 2: No immediate threat? Align horizontally with the nearest enemy to shoot them
    else if (!ctx.active_enemies.empty()) {
        auto& enemy = ctx.active_enemies.front(); // Simple heuristic: pick first enemy
        
        if (enemy.x < player.getPosX() - 4) {
            gamepad.pressLeft();
        } else if (enemy.x > player.getPosX() + 4) {
            gamepad.pressRight();
        }
        
        // Constantly fire weapons in a real-time shmup
        gamepad.pressFire();
    }

    // Priority 3: Keep moving forward if lagging behind screen scroll
    if (player.getPosY() > (WorldContext::kScreenHeight - 40)) {
        gamepad.pressUp();
    }
}

bool SimpleAIPlayer::findImmediateThreat(const Player& player, const WorldContext& ctx, WorldContext::EntityData& out_threat) {
    for (const auto& bullet : ctx.active_bullets) {
        // If bullet is within a vertical corridor directly above the AI
        if (std::abs(bullet.x - player.getPosY()) < 16 && bullet.y < player.getPosY() && (player.getPosY() - bullet.y) < 64) {
            out_threat = bullet;
            return true;
        }
    }
    return false;
}

}  // namespace KnightGame
