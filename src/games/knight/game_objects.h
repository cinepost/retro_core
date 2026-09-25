#ifndef __RETRO_CORE_TEST_GAME_OBJECTS_H
#define __RETRO_CORE_TEST_GAME_OBJECTS_H

#include <cstdint>
#include <cmath>
#include <memory>
#include <string>

#include "framework/ppu/ppu_msx.h"
#include "framework/oscillators.h"
#include "framework/game_engine/asset_manager.h"
#include "framework/game_engine/sound_player.h"

#include "game.h"

namespace KnightGame {

using SoundPlayer = GameEngine::SoundPlayer;
using MP3Stream = GameEngine::MP3Stream;

struct AABB {
    AABB() = default;
    AABB(int16_t _x, int16_t _y, int16_t _w, int16_t _h): x(_x), y(_y), w(_w), h(_h) {}
    
    void set(int16_t _x, int16_t _y, int16_t _w, int16_t _h) {
        x =_x; y =_y; w =_w; h =_h; 
    }

    bool isSingular() const { return w == 0 || h == 0; } 

    int16_t x = 0;
    int16_t y = 0;
    uint16_t w = 0;
    uint16_t h = 0;
};

class GameObject {
    public:
        using Sprite = SpriteList::Sprite;

        enum class EntityType {
            Unknown,
            Player,
            Projectile,
            PowerUp,
            WeaponUp
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

        GameObject(float start_x, float start_y)
            : mPosX(start_x), mPosY(start_y), mIsActive(true) 
        {}

        virtual ~GameObject() = default;

        [[nodiscard]] virtual EntityType getType() const { return EntityType::Unknown; }

        virtual void update(float dt, ObjectSpawner& spawner) = 0;
        virtual void draw(SpriteList& sprites) const = 0;

        float getPosX() const { return mPosX; }
        float getPosY() const { return mPosY; }

        template <typename T>
        T getPosX() const { return static_cast<T>(std::floor(getPosX())); }
        template <typename T>
        T getPosY() const { return static_cast<T>(std::floor(getPosY())); }
    
        bool isActive() const { return mIsActive; }
        void setActive(bool active) { mIsActive = active; }

        virtual const AABB& getCollisionAABB() const { return mCollisionAABB; } // relative to GameObject x, y position
        virtual const AABB& getObstacleAABB() const { return mObstacleAABB; } // relative to GameObject x, y position
        
        [[nodiscard]] bool checkCollisionAABB(const GameObject& other) const {
            const AABB& aabb_a = getCollisionAABB();
            const AABB& aabb_b = other.getCollisionAABB();

            return  ((getPosX<int16_t>() + aabb_a.x) < (other.getPosX<int16_t>() + aabb_b.x + aabb_b.w)) &&
                    ((getPosX<int16_t>() + aabb_a.x + aabb_a.w) > (other.getPosX<int16_t>() + aabb_b.x)) &&
                    ((getPosY<int16_t>() + aabb_a.y) < (other.getPosY<int16_t>() + aabb_b.y + aabb_b.h)) &&
                    ((getPosY<int16_t>() + aabb_a.y + aabb_a.h) > (other.getPosY<int16_t>() + aabb_b.y));
        }

        virtual void setPos(float x, float y) { mPosX = x; mPosY = y; }

    protected:
        // Float positioning for smooth movement
        float mPosX; 
        float mPosY;
        
        bool mIsActive;
        
        AABB mCollisionAABB; // AABB to check collisions. relative to x y coords
        AABB mObstacleAABB; // AABB to check collisions against map. relative to x y coords
};

class Upgrade: public GameObject {
    public:
        Upgrade(float x, float y): GameObject(x, y), mStateFreeze(true), mStateFreezeCounter(3), mSpawnX(x), mSpawnY(y), mRewardPointsCount(0), 
            mMoveOscillator(6, 0, 1, 1, 5), mSineOscillator(600.0f, -32.0f, 32.0f), mFlashingOscillator(18.0, 3.0f, 0.0f) {
            mCollisionAABB = {0, 0, 16, 16};
        }

        void update(float dt, ObjectSpawner& spawner) override {
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

        virtual void toggleState() {
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
        float mSpawnX;
        float mSpawnY;
        SawOscillator<float, float>         mFlashingOscillator;
        PulseOscillator<float, float>       mMoveOscillator; // drives upgrade object vertical movement movement
        SineWaveOscillator<float, float>    mSineOscillator; // controls horizontal movement
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

        WeaponUp(float x, float y): Upgrade(x, y), mState(State::PTS_1000) {
            updateSateData();
        }

        void update(float dt, ObjectSpawner& spawner) override final {
            Upgrade::update(dt, spawner);
        }

        void draw(SpriteList& sprites) const override final {
            float t = mFlashingOscillator.getValue();

            sprites.push({mPosX, mPosY, 52, (mState == State::PTS_1000 ? (t < 1.0 ? 1 : ((t < 2.0) ? 15 : 1)) : 1)});
            if(mState == State::PTS_1000) return;

            uint8_t color = t < 1.0 ? 0 : ((t < 2.0) ? 4 : 15);
            sprites.push({mPosX + mSpriteOffsetX, mPosY + mSpriteOffsetY, mSpritePatternIndex, color});
        }

        void toggleState() override final {
            Upgrade::toggleState();
            if(mStateFreeze) return;

            mState = (State)(((uint8_t)mState + 1) % (uint8_t)State::COUNT);
            updateSateData();
        }

        [[nodiscard]] State getState() const { return mState; }

        [[nodiscard]] virtual EntityType getType() const override final { return EntityType::WeaponUp; }

    private:
        void updateSateData() {
            mRewardPointsCount = 200;
            mSpriteOffsetX = 0.0f;
            mSpriteOffsetY = 0.0f;

            switch(mState) {
                case State::PTS_1000:
                    mRewardPointsCount = 1000;
                    mSpritePatternIndex = 0;
                    break;
                case State::DOUBLE_ARROW_200:
                    mSpriteOffsetX = 2.0f;
                    mSpriteOffsetY = 1.0f;
                    mSpritePatternIndex = 128;
                    break;
                case State::SWORD_200:
                    mSpriteOffsetX = 5.0f;
                    mSpritePatternIndex = 136;
                    break;
                case State::FIRE_ARROW_200:
                    mSpriteOffsetX = 4.0f;
                    mSpritePatternIndex = 132;
                    break;
                case State::FIRE_BALLS_200:
                    mSpritePatternIndex = 148;
                    break;
                case State::BOOMERANG_200:
                    mSpriteOffsetX = 6.0f;
                    mSpriteOffsetY = 3.0f;
                    mSpritePatternIndex = 96;
                    break;
                default:
                    assert(false);
                    break;
            }
        }

    private:
        State   mState;
        float   mSpriteOffsetX;
        float   mSpriteOffsetY;
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

        PowerUp(float x, float y): Upgrade(x, y), mState(State::PTS_1000) {
            mStateFreezeCounter = 1;
            updateSateData();
        }

        void update(float dt, ObjectSpawner& spawner) override final {
            Upgrade::update(dt, spawner);
        }

        void draw(SpriteList& sprites) const override final {
            sprites.push({mPosX, mPosY, 52, mBgColor});

            float t = mFlashingOscillator.getValue();
            uint8_t color = t < 1.0 ? 0 : ((t < 2.0) ? 4 : 15);

            sprites.push({mPosX + 5, mPosY + 3, 56, color});
        }

        void toggleState() override final {
            Upgrade::toggleState();
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

        Weapon(float x, float y, OwnerType whoFired, int16_t dmg): GameObject(x, y), mOwner(whoFired), mDamage(dmg) {

        }

        void update(float dt, ObjectSpawner& spawner) override {
            if(mDamage <= 0) setActive(false);
        }

        OwnerType   getOwner() const { return mOwner; }
        int16_t     getDamage() const { return mDamage; }
        
        void        decreaseDamage(int16_t delta) { 
            assert(delta >= 0);
            mDamage = std::min(0, mDamage - delta);
        }

    protected:
        OwnerType mOwner;
        int16_t mDamage;
};

class Character : public GameObject {
    public:
        static constexpr int16_t kMaxDamage = std::numeric_limits<int16_t>::max();
        Character(float x, float y, int16_t hp): GameObject(x, y), mHealth(hp), mMaxHealth(hp) {

        }

        virtual void update(float dt, ObjectSpawner& spawner) override {
            while (!mFiredWeapons.empty()) {
                spawner.spawnObject(std::move(mFiredWeapons.back()));
                mFiredWeapons.pop_back();
            }
            assert(mFiredWeapons.empty());
        }

        virtual void takeDamage(int16_t amount, bool force = false) {
            mHealth -= force ? mHealth : std::max(mHealth, std::min((int16_t)0, amount));
        }

    protected:
        int16_t mHealth;
        int16_t mMaxHealth;

        std::vector<std::unique_ptr<Weapon>> mFiredWeapons;

};

class Arrow: public Weapon {
    public:
        static const int16_t kDamage = 1;
        Arrow(float x, float y, float dx, float dy, OwnerType owner): Weapon(x, y, owner, kDamage), mDx(dx), mDy(dy), mDirection(getDirectionFromMovement(dx, dy)) 
        {
            setSpriteOffsetAndAABBFromDirection(dx, dy);
        }

        void update(float dt, ObjectSpawner& spawner) override final {
            Weapon::update(dt, spawner);
            mPosX += mDx * dt;
            mPosY += mDy * dt;
        }

        void draw(SpriteList& sprites) const override final {
            sprites.push({mPosX - mPivotX, mPosY - mPivotY, mSpritePatternIndex, (mOwner == OwnerType::Player ? 1 : 15)});
        }

        [[nodiscard]] virtual EntityType getType() const override final { return EntityType::Projectile; }

    private:
        void setSpriteOffsetAndAABBFromDirection(float dx, float dy) {
            switch(mDirection) {
                case Direction::UP:
                    mPivotX = 2.0f; mPivotY = 12.0f; mSpritePatternIndex = 64;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 5, 13);
                    break;
                case Direction::DOWN:
                    mPivotX = 2.0f; mPivotY = 0.0f; mSpritePatternIndex = 80;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 5, 13);
                    break; 
                case Direction::LEFT:
                    mPivotX = 12.0f; mPivotY = 2.0f; mSpritePatternIndex = 88;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 13, 5);
                    break;
                case Direction::RIGHT:
                    mPivotX = 0.0f; mPivotY = 2.0f; mSpritePatternIndex = 72;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 13, 5);
                    break;
                case Direction::LEFT_UP:
                    mPivotX = 10.0f; mPivotY = 10.0f; mSpritePatternIndex = 92;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 11, 11);
                    break;
                case Direction::LEFT_DOWN:
                    mPivotX = 10.0f; mPivotY = 0.0f; mSpritePatternIndex = 84;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 11, 11);
                    break; 
                case Direction::RIGHT_DOWN:
                    mPivotX = 0.0f; mPivotY = 0.0f; mSpritePatternIndex = 76;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 11, 11);
                    break;
                case Direction::RIGHT_UP:
                    mPivotX = 0.0f; mPivotY = 10.0f; mSpritePatternIndex = 68;
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

        float mPivotX;
        float mPivotY;

        uint16_t mSpritePatternIndex = 64;
};

class Player : public Character {
    public:
        enum class State {
            NORMAL,
            INVALNURABLE,
            INVISIBLE
        };

        enum class WeaponType {
            Arrow,
            DoubleArrow,
            FireArrow,
            FireBall,
            TripleFireBall,
            Knife,
            DoubleKnife,
            Boomerang
        };

        Player(float x, float y, uint8_t color) : Character(x, y, 100), mState(State::NORMAL), mPlayerColor(color), 
            mCurrentWeaponType(0), mHasShield(false), mScore(0), mLives(3),
            mWalkLegsOscillator(34.0f, 0, 1, 0.5 /* duty cycle */), 
            mWalkJumpOscillator(40.0f, 0.0f, 1.0f, 0.2 /* duty cycle */),
            mFiringCooldownTimer(20.0f /* 20 frames */),
            mDyingAnimationTimer(20.0f /* 20 animation frames */),
            mFlashingOscillator(20.0f, false, true, 0.5 /* duty cycle */)
        {
            mObstacleAABB = {4, 8, 8, 8}; // lower body half, no hands
            mCollisionAABB = {1, 0, 14, 16}; // full body
        }

        void update(float dt, ObjectSpawner& spawner) override final {
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
            mStateTimer.update(dt / 60.f); // state timer in seconds
            mFiringCooldownTimer.update(dt);
            mDyingAnimationTimer.update(dt * 0.2f /* 1/5 fps animation */);

            if(mState != State::NORMAL) {
                if(mStateTimer.hasExpired()) {
                    mState = State::NORMAL;
                } else {
                    mFiringCooldownTimer.reset(); // can shoot only in NORMAL state
                }
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
            float yy = mPosY - mWalkJumpOscillator.getValue();
            sprites.push({mPosX, yy, legsFlip ? 8 : 16, 1}); // outline

            bool flash = (mState != State::NORMAL && mStateTimer.getTimeRemaining() < 2.0f) ? mFlashingOscillator.getValue() : false;

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

            sprites.push({mPosX, yy-11, 0, 15}); //horns
        }

        void fire(const AssetManager& am, const SoundPlayer& sp) {
            if(!mCanShoot) return;

            switch(mWeaponType) {
                case WeaponType::Arrow:
                    static const MP3Stream sfxTrack(am.getAsset("sfx/shot_arrow.mp3"), false /* no loop */);
                    sp.playSFX(sfxTrack);
                    mFiredWeapons.push_back(std::make_unique<Arrow>(mPosX + 8.0f, mPosY, 0.0f, -4.0f, Weapon::OwnerType::Player));
                default:
                    
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

        void setWeaponType(WeaponType t) {
            mWeaponType = t;
        }

        void setState(State s, float timer_seconds = 15.0) {
            assert(timer_seconds >= 1.0f);
            mState = s;
            mStateTimer.start(std::max(1.0f, timer_seconds));
            if(mState != State::NORMAL) mCanShoot = false;
        }

        [[nodiscard]] float getStateTimeLeft() const { return (mState == State::NORMAL) ? 0.0f : mStateTimer.getTimeRemaining(); }

        [[nodiscard]] virtual EntityType getType() const override final { return EntityType::Player; }

        [[nodiscard]] int getScore() const { return mScore; }

        [[nodiscard]] int getLivesCount() const { return mLives; }

        [[nodiscard]] State getState() const { return mState; }

        [[nodiscard]] bool isDying() const { return mIsDying; }

    private:
        void drawDyingAnimation(SpriteList& sprites) const {
            int timeElapsed = static_cast<int>(std::floor(mDyingAnimationTimer.getTimeElapsed()));

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
        uint8_t mCurrentWeaponType;
        int     mScore;
        int     mLives;
        WeaponType mWeaponType = WeaponType::Arrow;
        bool    mHasShield;

        SquareWaveOscillator<float, uint8_t> mWalkLegsOscillator;
        SquareWaveOscillator<float, float>   mWalkJumpOscillator;
        SquareWaveOscillator<float, bool> mFlashingOscillator;

        GameEngine::Timer mStateTimer;
        GameEngine::Timer mFiringCooldownTimer;
        GameEngine::Timer mDyingAnimationTimer;

        bool mCanShoot = true; // prevent repeated input triggers
        bool mIsDying = false;
};

}  // namespace KnightGame

#endif  // __RETRO_CORE_TEST_GAME_OBJECTS_H

