#ifndef __RETRO_CORE_GAMES_KNIGHTMARE_AI_PLAYER_H
#define __RETRO_CORE_GAMES_KNIGHTMARE_AI_PLAYER_H

#include "game_objects.h"

#include <cstdint>

namespace KnightGame {

struct AiGamepad {
    AiGamepad() = default;

    void pressUp() { mBtnUp = true; }
    void pressDown() { mBtnDown = true; }
    void pressLeft() { mBtnLeft = true; }
    void pressRight() { mBtnRight = true; }
    void pressFire() { mBtnFire = true; }

    bool mBtnUp    = false;
    bool mBtnDown  = false;
    bool mBtnLeft  = false;
    bool mBtnRight = false;
    bool mBtnFire  = false;
};

struct WorldContext {
    static const int16_t kScreenHeight = 272;
    struct EntityData {
        int16_t     x, y;
        int16_t     width, height;
        int         type; // Enemy, Bullet, Power-up, Obstacle
    };

    bool    friend_player_alive;
    int16_t friend_player_x;
    int16_t friend_player_y;
    
    std::vector<EntityData> active_bullets;
    std::vector<EntityData> active_enemies;
    std::vector<EntityData> active_obstacles;
};

class SimpleAIPlayer{
    public:
        void update(const Player& player, const WorldContext& ctx, AiGamepad& gamepad);

    private:
        bool findImmediateThreat(const Player& player, const WorldContext& ctx, WorldContext::EntityData& out_threat);
};

}  // namespace KnightGame

#endif  // __RETRO_CORE_GAMES_KNIGHTMARE_AI_PLAYER_H

