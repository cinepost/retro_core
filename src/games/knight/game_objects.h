#ifndef __RETRO_CORE_GAMES_KNIGHTMARE_GAME_OBJECTS_H
#define __RETRO_CORE_GAMES_KNIGHTMARE_GAME_OBJECTS_H

#include <cstdint>
#include <cmath>
#include <memory>
#include <string>
#include <random>

#include "framework/ppu/ppu_msx.h"
#include "framework/oscillators.h"
#include "framework/game_engine/asset_manager.h"
#include "framework/game_engine/sound_player.h"

#include "game.h"
#include "sound_files.h"

namespace KnightGame {

using SoundPlayer = GameEngine::SoundPlayer;
using MP3Stream = GameEngine::MP3Stream;

template<typename T>
struct Pos {
    Pos(): mPosX(static_cast<T>(0)), mPosY(static_cast<T>(0)) {}

    T mPosX;
    T mPosY;

    void setPos(T x, T y) { mPosX = x; mPosY = y; }
};

struct AABB {
    AABB() = default;
    AABB(int _x, int _y, uint _w, uint _h): x(_x), y(_y), w(static_cast<int>(_w)), h(static_cast<int>(_h)) {}
    
    void set(int _x, int _y, uint _w, uint _h) {
        x =_x; y =_y; w = static_cast<int>(_w); h =static_cast<int>(_h); 
    }

    template<typename T>
    T getWidth() const { return static_cast<T>(w); }

    template<typename T>
    T getHeight() const { return static_cast<T>(h); }

    bool isSingular() const { return w == 0 || h == 0; } 

    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

enum class WeaponType {
    Arrow,
    DoubleArrow,
    FireArrow,
    FireBall,
    Knife,
    DoubleKnife,
    Boomerang,
    COUNT
};

class GameObject {
    public:
        using Sprite = SpriteList::Sprite;

        enum class EntityType {
            Unknown,
            Player,
            Enemy,
            Weapon,
            PowerUp,
            WeaponUp,
            Boss
        };

        enum class Direction {
            NONE,
            UP,
            DOWN,
            LEFT,
            RIGHT,
            LEFT_UP,
            LEFT_DOWN,
            RIGHT_UP,
            RIGHT_DOWN    
        };

        inline Direction getDirectionFromMovement(float dx, float dy) {
            constexpr float EPSILON = 0.01f;

            bool isMovingX = std::abs(dx) > EPSILON;
            bool isMovingY = std::abs(dy) > EPSILON;

            if (!isMovingX && !isMovingY) {
                return Direction::NONE; 
            }

            if (isMovingX && isMovingY) {
                // Diagonal Movements
                if (dx < 0.0f && dy < 0.0f) return Direction::LEFT_UP;
                if (dx < 0.0f && dy > 0.0f) return Direction::LEFT_DOWN;
                if (dx > 0.0f && dy < 0.0f) return Direction::RIGHT_UP;
                if (dx > 0.0f && dy > 0.0f) return Direction::RIGHT_DOWN;
            } else if (isMovingX) {
                return (dx < 0.0f) ? Direction::LEFT : Direction::RIGHT;
            } else if (isMovingY) {
                return (dy < 0.0f) ? Direction::UP : Direction::DOWN;
            }

            return Direction::NONE;
        }

        GameObject(int start_x, int start_y)
            : mPosX(start_x), mPosY(start_y), mIsActive(true) 
        {}

        virtual ~GameObject() = default;

        [[nodiscard]] virtual EntityType getType() const { return EntityType::Unknown; }

        [[nodiscard]] virtual bool isFreezable() const { return false; }

        virtual void update(int dt, ObjectSpawner& spawner) = 0;
        virtual void draw(SpriteList& sprites) const = 0;

        int getPosX() const { return mPosX; }
        int getPosY() const { return mPosY; }

        template <typename T>
        T getPosX() const { return static_cast<T>(getPosX()); }
        template <typename T>
        T getPosY() const { return static_cast<T>(getPosY()); }
    
        bool isActive() const { return mIsActive; }
        void setActive(bool active) { mIsActive = active; }

        virtual const AABB& getCollisionAABB() const { return mCollisionAABB; } // relative to GameObject x, y position
        virtual const AABB& getObstacleAABB() const { return mObstacleAABB; } // relative to GameObject x, y position
        
        [[nodiscard]] bool checkCollisionAABB(const GameObject& other) const {
            const AABB& aabb_a = getCollisionAABB();
            const AABB& aabb_b = other.getCollisionAABB();

            return  ((getPosX() + aabb_a.x) < (other.getPosX() + aabb_b.x + aabb_b.w)) &&
                    ((getPosX() + aabb_a.x + aabb_a.w) > (other.getPosX() + aabb_b.x)) &&
                    ((getPosY() + aabb_a.y) < (other.getPosY() + aabb_b.y + aabb_b.h)) &&
                    ((getPosY() + aabb_a.y + aabb_a.h) > (other.getPosY() + aabb_b.y));
        }

        virtual void setPos(int x, int y) { mPosX = x; mPosY = y; }

    protected:
        // Float positioning for smooth movement
        int mPosX; 
        int mPosY;
        
        bool mIsActive;
        
        AABB mCollisionAABB; // AABB to check collisions. relative to x y coords
        AABB mObstacleAABB; // AABB to check collisions against map. relative to x y coords
};

class Upgrade: public GameObject {
    public:
        Upgrade(int x, int y): GameObject(x, y), mStateFreeze(true), mStateFreezeCounter(3), mSpawnX(x), mSpawnY(y), mRewardPointsCount(0), 
            mMoveOscillator(6, 0, 1, 1, 5), mSineOscillator(600, -32, 32), mFlashingOscillator(18, 3.0f, 0.0f) {
            mCollisionAABB = {0, 0, 16, 16};
        }

        void update(int dt, ObjectSpawner& spawner) override {
            mMoveOscillator.update(dt);
            mSineOscillator.update(dt);
            mFlashingOscillator.update(dt);

            mPosY += mMoveOscillator.getValue();

            if(!mStateFreeze) mPosX = mSpawnX + mSineOscillator.getValue();
        }

        int getRewardPoints() { 
            int result = mRewardPointsCount; 
            mRewardPointsCount = 0; // avoid double spend
            return result;
        }

        virtual void toggleState(const AssetManager& am, const SoundPlayer& sp) {
            static const MP3Stream extraTakeSfxTrack(am.getAsset(kPowerUpHitSfxFileName), false /* no loop */);
            sp.playSFX(extraTakeSfxTrack);

            mSpawnX = mPosX;
            
            if(mStateFreeze) {
                mStateFreezeCounter--;
                if(mStateFreezeCounter <= 0) mStateFreeze = false;
            }
           
            mSineOscillator.swapMinMaxValues();
            mSineOscillator.reset();
        };

    protected:
        bool  mStateFreeze = true;
        int   mStateFreezeCounter = 3;
        int mSpawnX;
        int mSpawnY;
        SawOscillator<int, float>         mFlashingOscillator;
        PulseOscillator<int, int>         mMoveOscillator; // drives upgrade object vertical movement
        SineWaveOscillator<int, int>      mSineOscillator; // controls horizontal movement
        int mRewardPointsCount;
};

class WeaponUp : public Upgrade {
    public:
        enum class State: uint8_t { 
            PTS_1000, 
            DOUBLE_ARROW_200,
            SWORD_200,
            FIRE_ARROW_200,
            FIRE_BALLS_200,
            BOOMERANG_200,
            COUNT 
        };

        WeaponUp(int x, int y): Upgrade(x, y), mState(State::PTS_1000) {
            updateSateData();
        }

        void update(int dt, ObjectSpawner& spawner) override final {
            Upgrade::update(dt, spawner);
        }

        void draw(SpriteList& sprites) const override final {
            float t = mFlashingOscillator.getValue();

            sprites.push({mPosX, mPosY, 52, (mState == State::PTS_1000 ? (t < 1.0 ? 1 : ((t < 2.0) ? 15 : 1)) : 1)});
            if(mState == State::PTS_1000) return;

            uint8_t color = t < 1.0 ? 0 : ((t < 2.0) ? 4 : 15);
            sprites.push({mPosX + mSpriteOffsetX, mPosY + mSpriteOffsetY, mSpritePatternIndex, color});
        }

        void toggleState(const AssetManager& am, const SoundPlayer& sp) override final {
            Upgrade::toggleState(am, sp);
            if(mStateFreeze) return;

            mState = (State)(((uint8_t)mState + 1) % (uint8_t)State::COUNT);
            updateSateData();
        }

        [[nodiscard]] WeaponType getPlayerWeaponType() const {
            switch(mState) {
            case State::DOUBLE_ARROW_200:
                return WeaponType::DoubleArrow;
            case State::SWORD_200:
                return WeaponType::Knife;
            case State::FIRE_ARROW_200:
                return WeaponType::FireArrow;
            case State::FIRE_BALLS_200:
                return WeaponType::FireBall;
            case State::BOOMERANG_200:
                return WeaponType::Boomerang;
            case State::PTS_1000:
            default:
                return WeaponType::COUNT; // Caution! COUNT returned as "no weapon type" here. e.g. State::PTS_1000 does not change player's weapon
            }
        }

        [[nodiscard]] State getState() const { return mState; }

        [[nodiscard]] virtual EntityType getType() const override final { return EntityType::WeaponUp; }

    private:
        void updateSateData() {
            mRewardPointsCount = 200;
            mSpriteOffsetX = 0;
            mSpriteOffsetY = 0;

            switch(mState) {
                case State::PTS_1000:
                    mRewardPointsCount = 1000;
                    mSpritePatternIndex = 0;
                    break;
                case State::DOUBLE_ARROW_200:
                    mSpriteOffsetY = -1;
                    mSpritePatternIndex = 128;
                    break;
                case State::SWORD_200:
                    mSpritePatternIndex = 136;
                    break;
                case State::FIRE_ARROW_200:
                    mSpritePatternIndex = 132;
                    break;
                case State::FIRE_BALLS_200:
                    mSpritePatternIndex = 148;
                    break;
                case State::BOOMERANG_200:
                    mSpriteOffsetX = 4;
                    mSpriteOffsetY = 3;
                    mSpritePatternIndex = 96;
                    break;
                default:
                    assert(false);
                    break;
            }
        }

    private:
        State   mState;
        int mSpriteOffsetX;
        int mSpriteOffsetY;
        uint8_t mSpritePatternIndex;
};

class PowerUp : public Upgrade {
    public:
        enum class State: uint8_t { 
            PTS_1000, 
            PTS_200,
            SHIELD_200,
            INVALNURABILE_200,
            INVISIBLE_200,
            COUNT 
        };

        PowerUp(int x, int y): Upgrade(x, y), mState(State::PTS_1000) {
            mStateFreezeCounter = 1;
            updateSateData();
        }

        void update(int dt, ObjectSpawner& spawner) override final {
            Upgrade::update(dt, spawner);
        }

        void draw(SpriteList& sprites) const override final {
            sprites.push({mPosX, mPosY, 52, mBgColor});

            float t = mFlashingOscillator.getValue();
            uint8_t color = t < 1.0 ? 0 : ((t < 2.0) ? 4 : 15);

            sprites.push({mPosX + 5, mPosY + 3, 56, color});
        }

        void toggleState(const AssetManager& am, const SoundPlayer& sp) override final {
            Upgrade::toggleState(am, sp);
            mState = (State)(((uint8_t)mState + 1) % (uint8_t)State::COUNT);
            updateSateData();
        }

        [[nodiscard]] State getState() const { return mState; }

        [[nodiscard]] virtual EntityType getType() const override final { return EntityType::PowerUp; }

    private:
        void updateSateData() {
            mRewardPointsCount = 200;
            switch(mState) {
                case State::PTS_1000:
                    mRewardPointsCount = 1000;
                    mBgColor = 1;
                    break;
                case State::PTS_200:
                    mBgColor = 4;
                    break;
                case State::SHIELD_200:
                    mBgColor = 7;
                    break;
                case State::INVALNURABILE_200:
                    mBgColor = 6;
                    break;
                case State::INVISIBLE_200:
                    mBgColor = 15;
                    break;
                default:
                    assert(false);
                    break;
            }
        }

    private:
        State   mState;
        uint8_t mBgColor;
};

class Weapon : public GameObject {
    public:
        enum class OwnerType { Player, Enemy };

        Weapon(int x, int y, OwnerType whoFired, int16_t dmg): GameObject(x, y), mOwner(whoFired), mDamage(dmg) {
            mCollisionAABB.set(0, 0, 16, 16); // default
        }

        void update(int dt, ObjectSpawner& spawner) override {
            if(mDamage <= 0) setActive(false);
        }

        [[nodiscard]] OwnerType   getOwner() const { return mOwner; }
        [[nodiscard]] int16_t     getDamage() const { return mDamage; }
        
        void        decreaseDamage(int delta) { 
            assert(delta >= 0);
            mDamage = std::max(0, mDamage - delta);
        }

        [[nodiscard]] virtual bool isPiercing() const { return false; }

        [[nodiscard]] virtual bool isProjectile() const { return true; }

        [[nodiscard]] virtual EntityType getType() const override { return EntityType::Weapon; }

    protected:
        OwnerType mOwner;
        int16_t mDamage;
};

class Character: public GameObject {
    public:
        static constexpr int16_t kMaxDamage = std::numeric_limits<int16_t>::max();
        Character(int x, int y, int16_t hp): GameObject(x, y), mHealth(hp), mMaxHealth(hp) {

        }

        virtual void update(int dt, ObjectSpawner& spawner) override {
            while (!mFiredWeapons.empty()) {
                spawner.spawnObject(std::move(mFiredWeapons.back()));
                mFiredWeapons.pop_back();
            }
            assert(mFiredWeapons.empty());
        }

        [[nodiscard]] virtual bool takeDamage(int16_t amount, const AssetManager& am, const SoundPlayer& sp, bool force = false) {
            if(mHealth <= 0) return false;

            mHealth -= force ? mHealth : std::min(mHealth, std::max((int16_t)0, amount));
            return true;
        }

        [[nodiscard]] bool getHealth() const noexcept { return mHealth; }

    protected:
        int16_t mHealth;
        int16_t mMaxHealth;

        std::vector<std::unique_ptr<Weapon>> mFiredWeapons;

};

class BurningAshes: public GameObject {
    public:
        BurningAshes(int x, int y, int duration = 60): GameObject(x, y), mTimer(duration), 
            mFrameCount(0), mAnimPhaseShiftOscillator(4, 0, 1, 0.5) 
        {
            mCollisionAABB = {1, 0, 13, 16};
        }

        void update(int dt, ObjectSpawner& spawner) override final {
            mTimer.update(dt);
            if(mTimer.hasExpired()) {
                setActive(false);
                return;
            }

            mAnimPhaseShiftOscillator.update(dt);
            mFrameCount++;
        }

        void draw(SpriteList& sprites) const override final {
            int phase = mFrameCount / 12 + mAnimPhaseShiftOscillator.getValue();

            switch(phase) {
                case 0:
                    sprites.push({mPosX, mPosY, 176, 8});
                    sprites.push({mPosX+4, mPosY+5, 172, 10});
                    break;
                case 1:
                    sprites.push({mPosX+3, mPosY-1, 184, 8});
                    sprites.push({mPosX+5, mPosY+7, 180, 10});
                    break;
                case 2:
                    sprites.push({mPosX+3, mPosY+3, 188, 8});
                    break;
                default:
                    break;
            }
        }

    protected:
        GameEngine::Timer mTimer;

        int mFrameCount;
        SquareWaveOscillator<int, int>    mAnimPhaseShiftOscillator;
};


class Enemy: public Character {
    public:
        Enemy(int x, int y, int16_t hp): Character(x, y, hp) {

        }

        virtual void update(int dt, ObjectSpawner& spawner) override {
            if(mHealth <= 0) {
                spawner.spawnObject(std::make_unique<BurningAshes>(mPosX, mPosY));
                setActive(false);
                return;
            }

            Character::update(dt, spawner);
        }

        [[nodiscard]] virtual bool takeDamage(int16_t amount, const AssetManager& am, const SoundPlayer& sp, bool force = false) override {
            const bool result = Character::takeDamage(amount, am, sp, force);

            if(mHealth <= 0) {
                static const MP3Stream sfxTrack(am.getAsset(kEnemyKillSfxFileName), false /* no loop */);
                sp.playSFX(sfxTrack);
            }

            return result;
        }

        [[nodiscard]] virtual EntityType getType() const override { return EntityType::Enemy; }

        [[nodiscard]] virtual bool isFreezable() const override { return true; }
};

class Slime: public Enemy {
    public:

        Slime(int x, int y, int16_t hp = 1): Enemy(x, y, hp), mAnimOscillator(12, 0, 1, 1, 0), mMoveOscillator(8, 0, 1, 1, 0) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dist(0, 3);

            mAnimFrame = dist(gen);
            mAnimOscillator.setPulsePosition(dist(gen) * 3); // slight out of sync animation cycling
            mMoveOscillator.setPulsePosition(dist(gen) * 2); // slight out of sync movement
            mCollisionAABB.set(0, 0, 16, 16);
        }

        void update(int dt, ObjectSpawner& spawner) override final {
            Enemy::update(dt, spawner);

            if(dt <= 0) return;

            mAnimOscillator.update(dt);
            mMoveOscillator.update(dt);

            mAnimFrame += mAnimOscillator.getValue();
            
            switch(mAnimFrame % 4) {
                case 0:
                    mCollisionAABB.set(1, 2, 14, 11);
                    break;
                case 1:
                case 3:
                    mCollisionAABB.set(2, 2, 12, 12);
                    break;
                case 2:
                default:
                    mCollisionAABB.set(3, 1, 11, 14);
                    break;
            }

            mPosY += mMoveOscillator.getValue();
        }

        void draw(SpriteList& sprites) const override final {
            switch(mAnimFrame % 4) {
                case 0:
                    sprites.push({mPosX + 3, mPosY + 4, 156, 15});
                    sprites.push({mPosX, mPosY, 272, 1});
                    break;
                case 1:
                case 3:
                    sprites.push({mPosX + 4, mPosY + 4, 156, 15});
                    sprites.push({mPosX, mPosY, 276, 1});
                    break;
                case 2:
                default:
                    sprites.push({mPosX + 4, mPosY + 3, 156, 15});
                    sprites.push({mPosX, mPosY, 280, 1});
                    break;
            }
        }

    private:
        int                                   mAnimFrame = 0;
        PulseOscillator<int, int>             mAnimOscillator; // sprite switch
        PulseOscillator<int, int>             mMoveOscillator; // slime movement
};


// Configurable projectile for FireArrow, DoubleArrow, Knife, DoubleKnife types
template<WeaponType PT>
class Projectile: public Weapon {
    public:
        Projectile(int x, int y, int dx, int dy, OwnerType owner): Weapon(x, y, owner, projectileTypeToDamage()), mDx(dx), mDy(dy)
        {
            mPosY -= 8;

            if constexpr (PT == WeaponType::FireBall) {
                mColor = 10;
                mSpritePatternIndex = 144;
                mCollisionAABB.set(6, 2, 4, 10);
            } else if constexpr (PT == WeaponType::FireArrow) {
                mColor = 15;
                mSpritePatternIndex = 132;
                mCollisionAABB.set(4, 1, 8, 14);
            } else if constexpr (PT == WeaponType::Knife) {
                mColor = 15;
                mSpritePatternIndex = 136;
                mCollisionAABB.set(5, 0, 6, 15);
            } else if constexpr (PT == WeaponType::DoubleKnife) {
                mColor = 15;
                mSpritePatternIndex = 140;
                mCollisionAABB.set(1, 0, 13, 15);
            } else if constexpr (PT == WeaponType::DoubleArrow) {
                mColor = 1;
                mSpritePatternIndex = 128;
                mCollisionAABB.set(2, 2, 12, 13);
            } else {
                static_assert(false);
            }
        }

        void update(int dt, ObjectSpawner& spawner) override final {
            Weapon::update(dt, spawner);
            mPosX += mDx * dt;
            mPosY += mDy * dt;
        }

        void draw(SpriteList& sprites) const override final {
            if constexpr (PT == WeaponType::FireBall) {
                sprites.push({mPosX, mPosY - 1, mSpritePatternIndex, 9});
                sprites.push({mPosX, mPosY, mSpritePatternIndex, 10});
            } else {
                sprites.push({mPosX, mPosY, mSpritePatternIndex, mColor});
            }
        }

    private:
        static constexpr int16_t projectileTypeToDamage() {
            if constexpr (PT == WeaponType::FireArrow) {
                return 3;
            } else if constexpr (PT == WeaponType::Knife) {
                return 2;
            } else if constexpr (PT == WeaponType::DoubleKnife) {
                return 4;
            } else if constexpr (PT == WeaponType::DoubleArrow) {
                return 2;
            } else if constexpr (PT == WeaponType::FireBall) {
                return 2;
            } else {
                static_assert(false);
            }
            return 0;
        }

    private:
        int mDx;
        int mDy;
        uint8_t mColor = 15;
        uint16_t mSpritePatternIndex;

};

class Arrow: public Weapon {
    public:
        static const int16_t kDamage = 1;
        Arrow(int x, int y, float dx, float dy, OwnerType owner): Weapon(x, y, owner, kDamage), mDx(dx), mDy(dy), mDirection(getDirectionFromMovement(dx, dy)) 
        {
            setSpriteOffsetAndAABBFromDirection();
        }

        void update(int dt, ObjectSpawner& spawner) override final {
            Weapon::update(dt, spawner);
            mPosX += static_cast<int>(std::floor(mDx * dt));
            mPosY += static_cast<int>(std::floor(mDy * dt));
        }

        void draw(SpriteList& sprites) const override final {
            sprites.push({mPosX - mPivotX, mPosY - mPivotY, mSpritePatternIndex, (mOwner == OwnerType::Player ? 1 : 15)});
        }

    private:
        void setSpriteOffsetAndAABBFromDirection() {
            switch(mDirection) {
                case Direction::UP:
                    mPivotX = 2; mPivotY = 12; mSpritePatternIndex = 64;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 5, 13);
                    break;
                case Direction::DOWN:
                    mPivotX = 2; mPivotY = 0; mSpritePatternIndex = 80;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 5, 13);
                    break; 
                case Direction::LEFT:
                    mPivotX = 12; mPivotY = 2; mSpritePatternIndex = 88;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 13, 5);
                    break;
                case Direction::RIGHT:
                    mPivotX = 0; mPivotY = 2; mSpritePatternIndex = 72;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 13, 5);
                    break;
                case Direction::LEFT_UP:
                    mPivotX = 10; mPivotY = 10; mSpritePatternIndex = 92;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 11, 11);
                    break;
                case Direction::LEFT_DOWN:
                    mPivotX = 10; mPivotY = 0; mSpritePatternIndex = 84;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 11, 11);
                    break; 
                case Direction::RIGHT_DOWN:
                    mPivotX = 0; mPivotY = 0; mSpritePatternIndex = 76;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 11, 11);
                    break;
                case Direction::RIGHT_UP:
                    mPivotX = 0; mPivotY = 10; mSpritePatternIndex = 68;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 11, 11);
                    break; 
                default:
                    assert(false);
                    break;
            }
        }

        float mDx;
        float mDy;

        Direction mDirection;

        int mPivotX;
        int mPivotY;

        uint16_t mSpritePatternIndex = 64;
};

class Boomerang: public Weapon {
    public:
        static const int16_t kDamage = 2;
        Boomerang(int x, int y, float dx, float dy, OwnerType owner, const GameObject* pTrackingObject = nullptr): Weapon(x, y, owner, kDamage), 
            mDx(dx), mDy(dy), 
            mpTrackingObject(pTrackingObject),
            mTimer(120.f), mMoveOscillator(120, 0.f, 64.0f)
        {
            if(pTrackingObject) {
                mReturnPosX = pTrackingObject->getPosX();
                mReturnPosY = pTrackingObject->getPosY();
                mTrackingOffsetX = x - mReturnPosX;
                mTrackingOffsetY = y - mReturnPosY;
            } else {
                mReturnPosX = x;
                mReturnPosY = y;
            }

            mCollisionAABB = {0, 0, 10, 10};
        }

        void update(int dt, ObjectSpawner& spawner) override final {
            if(mTimer.hasExpired()) {
                setActive(false);
                return;
            }

            if(mpTrackingObject && mpTrackingObject->isActive()) {
                mReturnPosX = mpTrackingObject->getPosX();
                mReturnPosY = mpTrackingObject->getPosY();
            }

            Weapon::update(dt, spawner);
            mMoveOscillator.update(1);
            mAnimOscillator.update(1);
            mTimer.update(1.0f);

            float move_delta = mMoveOscillator.getValue();
            mPosX = mReturnPosX + mTrackingOffsetX + static_cast<int>(std::floor(mDx * move_delta));
            mPosY = mReturnPosY + mTrackingOffsetY + static_cast<int>(std::floor(mDy * move_delta));
        }

        void draw(SpriteList& sprites) const override final {
            sprites.push({mPosX, mPosY, mAnimOscillator.getValue(), 1});
        }

        [[nodiscard]] virtual bool isProjectile() const override final { return false; }

        [[nodiscard]] virtual bool isPiercing() const override final { return true; }

    private:
        float mDx;
        float mDy;

        int mReturnPosX = 0;
        int mReturnPosY = 0;
        
        int mTrackingOffsetX = 0;
        int mTrackingOffsetY = 0;

        const GameObject* mpTrackingObject;

        GameEngine::Timer mTimer;
        BounceOscillator<int, float>    mMoveOscillator;
        CycleOscillator<int, uint16_t>  mAnimOscillator = {24, {96, 100, 104, 108}};

        uint16_t mSpritePatternIndex = 64;
};


class Player : public Character {
    public:
        enum class State {
            NORMAL,
            INVALNURABLE,
            INVISIBLE
        };

        Player(int x, int y, uint8_t color) : Character(x, y, 100), mState(State::NORMAL), mPlayerColor(color), mCurrentWeaponType(WeaponType::Boomerang), mScore(0), mLives(3), mHasShield(false),
            mWalkLegsOscillator(34, 0, 1, 0.5 /* duty cycle */), 
            mWalkJumpOscillator(40, 0, 1, 0.2 /* duty cycle */),
            mFlashingOscillator(20, false, true, 0.5 /* duty cycle */),
            mFiringCooldownTimer(20 /* 20 frames */),
            mDyingAnimationTimer(20 * 5 /* 20 animation frames at 1/5 fps*/)
        {
            mObstacleAABB = {4, 8, 8, 8}; // lower body half, no hands
            mCollisionAABB = {1, 0, 14, 16}; // full body
        }

        void update(int dt, ObjectSpawner& spawner) override final {
            Character::update(dt, spawner);

            if(mHealth <= 0) {
                if(!mIsDying) {
                    mIsDying = true;
                    mDyingAnimationTimer.reset();
                } else {
                    if(mDyingAnimationTimer.hasExpired()) setActive(false);
                }
            }

            mWalkLegsOscillator.update(dt);
            mWalkJumpOscillator.update(dt);
            mFlashingOscillator.update(dt);
            mStateTimer.update(dt);
            mFiringCooldownTimer.update(dt);
            mDyingAnimationTimer.update(dt);

            if(mState != State::NORMAL && mStateTimer.hasExpired()) {
                mState = State::NORMAL;
            }  

            if(!mCanShoot && mFiringCooldownTimer.hasExpired()) {
                mFiringCooldownTimer.reset();
                mCanShoot = true;
            }
        }

        void draw(SpriteList& sprites) const override final {
            if(mIsDying) {
                drawDyingAnimation(sprites);
                return;
            }

            auto legsFlip = mWalkLegsOscillator.getValue();
            int yy = mPosY - mWalkJumpOscillator.getValue();
            sprites.push({mPosX, yy, legsFlip ? 8 : 16, 1}); // outline

            bool flash = (mState != State::NORMAL && mStateTimer.getTimeRemaining() < 120) ? mFlashingOscillator.getValue() : false;

            // infill
            switch(mState) {
                case State::INVALNURABLE:
                    sprites.push({mPosX, yy, legsFlip ? 4 : 12, flash ? mPlayerColor : 6});
                case State::INVISIBLE:
                    if(flash) sprites.push({mPosX, yy, legsFlip ? 4 : 12, mPlayerColor});
                    break;
                case State::NORMAL:
                default:
                    sprites.push({mPosX, yy, legsFlip ? 4 : 12, mPlayerColor});
                    if(mHasShield) {
                        sprites.push({mPosX, yy - 18, 44, 15});
                        sprites.push({mPosX, yy - 18, 48, mPlayerColor});
                    }
                    break;
            }

            sprites.push({mPosX, yy - 11, 0, 15}); //horns
        }

        void fire(const AssetManager& am, const SoundPlayer& sp) {
            if(!mCanShoot || mState == State::INVALNURABLE) return;

            switch(mCurrentWeaponType) {
                case WeaponType::Boomerang:
                    {
                        static const MP3Stream sfxTrack(am.getAsset(kShotBoomerangSfxFileName), false /* no loop */);
                        sp.playSFX(sfxTrack);
                        mFiredWeapons.push_back(std::make_unique<Boomerang>(mPosX + 4, mPosY, 0.0f, -2.0f, Weapon::OwnerType::Player, this /* track player position */));
                    }
                    break;
                case WeaponType::FireArrow:
                    {
                        mFiredWeapons.push_back(std::make_unique<Projectile<WeaponType::FireArrow>>(mPosX, mPosY, 0, -4, Weapon::OwnerType::Player));
                    }
                    break;
                case WeaponType::DoubleArrow:
                    {
                        mFiredWeapons.push_back(std::make_unique<Projectile<WeaponType::DoubleArrow>>(mPosX, mPosY, 0, -4, Weapon::OwnerType::Player));
                    }
                    break;
                case WeaponType::DoubleKnife:
                    {
                        mFiredWeapons.push_back(std::make_unique<Projectile<WeaponType::DoubleKnife>>(mPosX, mPosY, 0, -4, Weapon::OwnerType::Player));
                    }
                    break;
                case WeaponType::Knife:
                    {
                        mFiredWeapons.push_back(std::make_unique<Projectile<WeaponType::Knife>>(mPosX, mPosY, 0, -4, Weapon::OwnerType::Player));
                    }
                    break;
                case WeaponType::FireBall:
                    {
                        mFiredWeapons.push_back(std::make_unique<Projectile<WeaponType::FireBall>>(mPosX, mPosY,-1, -3, Weapon::OwnerType::Player));
                        mFiredWeapons.push_back(std::make_unique<Projectile<WeaponType::FireBall>>(mPosX, mPosY, 0, -4, Weapon::OwnerType::Player));
                        mFiredWeapons.push_back(std::make_unique<Projectile<WeaponType::FireBall>>(mPosX, mPosY, 1, -3, Weapon::OwnerType::Player));
                    }
                    break;
                case WeaponType::Arrow:
                default:
                    {
                        static const MP3Stream sfxTrack(am.getAsset(kShotArrowSfxFileName), false /* no loop */);
                        sp.playSFX(sfxTrack);
                        mFiredWeapons.push_back(std::make_unique<Arrow>(mPosX + 8, mPosY, 0.0f, -4.0f, Weapon::OwnerType::Player));
                    }
                    break;
            }
            
            mCanShoot = false;
            mFiringCooldownTimer.reset();
        }

        void giveShield() { mHasShield = true; }

        void adjustScore(int scoreDelta) {
            mScore = std::max(0, mScore + scoreDelta);
        }

        void adjustLives(int livesDelta) {
            mLives = std::max(0, mLives + livesDelta);
        }

        void setState(State s, int timer_seconds = 15.0) {
            assert(timer_seconds >= 1.0f);
            mState = s;
            mStateTimer.start(std::max(1, timer_seconds) * 60 /* 60 fps */);
            if(mState == State::INVALNURABLE) mCanShoot = false;
        }

        void setWeaponType(WeaponType weaponType) {
            if(weaponType == WeaponType::COUNT || weaponType == mCurrentWeaponType) return;
            mCurrentWeaponType = weaponType;
        }

        [[nodiscard]] float getStateTimeLeft() const { return (mState == State::NORMAL) ? 0.0f : mStateTimer.getTimeRemaining(); }

        [[nodiscard]] virtual EntityType getType() const override final { return EntityType::Player; }

        [[nodiscard]] int getScore() const { return mScore; }

        [[nodiscard]] int getLivesCount() const { return mLives; }

        [[nodiscard]] State getState() const { return mState; }

        [[nodiscard]] bool isDying() const { return mIsDying; }

    private:
        void drawDyingAnimation(SpriteList& sprites) const {
            int timeElapsed = static_cast<int>(mDyingAnimationTimer.getTimeElapsed()) / 5;

            if(timeElapsed < 15) {
                // flashing anim
                sprites.push({mPosX, mPosY, 8, 1}); // outline
                sprites.push({mPosX, mPosY, 4, timeElapsed % 2 ? mPlayerColor : 15 }); // fill
                sprites.push({mPosX, mPosY-11, 0, 15}); //horns
            } else {
                // burning anim
                sprites.push({mPosX, mPosY, 40, 1}); // ground shadow
                if(timeElapsed < 19) {
                    sprites.push({mPosX, mPosY, 24 + (timeElapsed - 15) * 4, 15}); // fire
                }
            }
        }


    private:
        State   mState;
        uint8_t mPlayerColor;
        int     mScore;
        int     mLives;
        WeaponType mCurrentWeaponType = WeaponType::Boomerang;
        bool    mHasShield;

        SquareWaveOscillator<int, uint8_t> mWalkLegsOscillator;
        SquareWaveOscillator<int, int>     mWalkJumpOscillator;
        SquareWaveOscillator<int, bool>    mFlashingOscillator;

        GameEngine::Timer mStateTimer;
        GameEngine::Timer mFiringCooldownTimer;
        GameEngine::Timer mDyingAnimationTimer;

        bool mCanShoot = true; // prevent repeated input triggers
        bool mIsDying = false;
};

}  // namespace KnightGame

#endif  // __RETRO_CORE_GAMES_KNIGHTMARE_GAME_OBJECTS_H

