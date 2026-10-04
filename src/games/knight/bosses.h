#ifndef __RETRO_CORE_GAMES_KNIGHTMARE_BOSSES_H
#define __RETRO_CORE_GAMES_KNIGHTMARE_BOSSES_H

#include "game_objects.h"

namespace KnightGame {

/* Bosses health points

Medusa = 20
GrimReaper = 30
SilverKnight = 40
DemonFace = 50
FireDragon = 60
RedKnight = 70
TwinGiantHeads = 80
Hudnos = 100

*/

class Boss: public Character {
    public:
        Boss(int x, int y, int16_t hp, uint16_t boss_sprites_start_index, const std::array<Player*, 2>&  players): Character(x, y, hp), 
            mPlayers(players), mSpritesStartIndex(boss_sprites_start_index), mWakeUpTimer(360.0f /* 6 sec safety */, [this] { this->mIsAwake = true; }) 
        {

        }

        [[nodiscard]] virtual EntityType getType() const { return EntityType::Boss; }
    
        virtual void update(int dt, ObjectSpawner& spawner) override final {
            mWakeUpTimer.update(dt);

            if(!mIsAwake) return;

            Character::update(dt, spawner);
            updateImpl(dt, spawner);
        }

        virtual void updateImpl(int dt, ObjectSpawner& spawner) = 0;

        void wakeUp() {
            mWakeUpTimer.start(60.0f /* one sec delay */, [this] { this->mIsAwake = true; });
        }

        [[nodiscard]] bool isAwake() const { return mIsAwake; }

    protected:
        // distance offset to closest player in horizontal plane
        [[nodiscard]] int getHorizontalOffsetToClosestPlayer() const {
            assert(!mCollisionAABB.isSingular());

            if(!mPlayers[0] && !mPlayers[1]) return 0;

            int boss_centroid_x = mPosX + (mCollisionAABB.w - mCollisionAABB.x) / 2;
            
            uint prev_offset_abs = std::numeric_limits<uint>::max();
            int result = 0;

            for(const Player* pPlayer: mPlayers) {
                if(!pPlayer || !pPlayer->isActive() || pPlayer->isDying()) continue;
                const AABB& aabb = pPlayer->getCollisionAABB();
                int player_centroid_x = pPlayer->getPosX() + (aabb.w - aabb.x) / 2;
                int offset = boss_centroid_x - player_centroid_x;
                if(std::abs(offset) < prev_offset_abs) {
                    prev_offset_abs = std::abs(offset);
                    result = offset;
                }
            }

            return result;
        }

    protected:
        bool  mIsAwake = false;
        const std::array<Player*, 2>& mPlayers; // for players tracking
        uint16_t mSpritesStartIndex;

        GameEngine::Timer mWakeUpTimer; // pause (frame based) before visible boss awakes
};


class Medusa_Boss: public Boss {
    public:
        Medusa_Boss(int x, int y, uint16_t boss_sprites_start_index, const std::array<Player*, 2>&  players): Boss(x, y, 20, boss_sprites_start_index, players),
            mCurlesOscillator(60u, true, false, 0.5), 
            mEyesOscillator(30u, true, false, 0.5),
            mMoveOscillator(6, 0, 1, 1, 5)  
        {
            mCollisionAABB = {4, 4, 40, 40};
        }

        virtual void updateImpl(int dt, ObjectSpawner& spawner) override final {
            mCurlesOscillator.update(dt);
            mEyesOscillator.update(dt);
            mMoveOscillator.update(dt);

            int offset_to_player = getHorizontalOffsetToClosestPlayer();
            if(offset_to_player < 0) {
                mPosX += mMoveOscillator.getValue();
            } else if (offset_to_player > 0) {
                mPosX -= mMoveOscillator.getValue();
            }
        }

        void draw(SpriteList& sprites) const override final {

            // back curly part
            if(mCurlesOscillator.getValue()) {
                sprites.push({mPosX, mPosY, mSpritesStartIndex, 1});
                sprites.push({mPosX + 16, mPosY, mSpritesStartIndex + 4, 1});
                sprites.push({mPosX + 32, mPosY, mSpritesStartIndex + 8, 1});

                sprites.push({mPosX, mPosY + 16, mSpritesStartIndex + 32, 1});
                sprites.push({mPosX + 16, mPosY + 16, mSpritesStartIndex + 36, 1});
                sprites.push({mPosX + 32, mPosY + 16, mSpritesStartIndex + 40, 1});

                sprites.push({mPosX, mPosY + 32, mSpritesStartIndex + 64, 1});
                sprites.push({mPosX + 16, mPosY + 32, mSpritesStartIndex + 68, 1});
                sprites.push({mPosX + 32, mPosY + 32, mSpritesStartIndex + 72, 1});
            } else {
                sprites.push({mPosX, mPosY, mSpritesStartIndex + 12, 1});
                sprites.push({mPosX + 16, mPosY, mSpritesStartIndex + 16, 1});
                sprites.push({mPosX + 32, mPosY, mSpritesStartIndex + 20, 1});

                sprites.push({mPosX, mPosY + 16, mSpritesStartIndex + 44, 1});
                sprites.push({mPosX + 16, mPosY + 16, mSpritesStartIndex + 36, 1});
                sprites.push({mPosX + 32, mPosY + 16, mSpritesStartIndex + 52, 1});

                sprites.push({mPosX, mPosY + 32, mSpritesStartIndex + 76, 1});
                sprites.push({mPosX + 16, mPosY + 32, mSpritesStartIndex + 68, 1});
                sprites.push({mPosX + 32, mPosY + 32, mSpritesStartIndex + 80, 1});
            }

            // red dress
            sprites.push({mPosX + 8, mPosY + 20, mSpritesStartIndex + 24, 6});
            sprites.push({mPosX + 24, mPosY + 20, mSpritesStartIndex + 28, 6});

            sprites.push({mPosX + 8, mPosY + 36, mSpritesStartIndex + 56, 6});
            sprites.push({mPosX + 24, mPosY + 36, mSpritesStartIndex + 60, 6});

            // hands and head
            sprites.push({mPosX, mPosY + 12, mSpritesStartIndex + 84, 11});
            sprites.push({mPosX + 16, mPosY + 12, mSpritesStartIndex + 88, 11});
            sprites.push({mPosX + 32, mPosY + 12, mSpritesStartIndex + 92, 11});

            // flashing eyes
            if(mEyesOscillator.getValue()) {
                sprites.push({mPosX + 16, mPosY + 12, mSpritesStartIndex + 48, 8});
            }
        }

    private:
        SquareWaveOscillator<int, bool>    mCurlesOscillator;
        SquareWaveOscillator<int, bool>    mEyesOscillator;

        PulseOscillator<int, int>          mMoveOscillator; // drives boss horizontal movement
};


}  // namespace KnightGame

#endif  // __RETRO_CORE_GAMES_KNIGHTMARE_BOSSES_H

