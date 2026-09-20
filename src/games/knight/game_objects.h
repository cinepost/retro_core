#ifndef __RETRO_CORE_TEST_GAME_OBJECTS_H
#define __RETRO_CORE_TEST_GAME_OBJECTS_H

#include <cstdint>
#include <cmath>
#include <memory>

#include "framework/ppu/ppu_msx.h"
#include "framework/oscillators.h"

#include "game.h"

namespace KnightGame {

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
            Projectile
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
            : x(start_x), y(start_y), mIsActive(true) 
        {}

        virtual ~GameObject() = default;

        [[nodiscard]] virtual EntityType getType() const { return EntityType::Unknown; }

        virtual void update(float dt, ObjectSpawner& spawner) = 0;
        virtual void draw(SpriteList& sprites) const = 0;
        virtual uint16_t getWidth() const { return 0; }
        virtual uint16_t getHeight() const { return 0; }

        float getPosX() const { return x; }
        float getPosY() const { return y; }

        template <typename T>
        T getPosX() const { return static_cast<T>(std::floor(x)); }
        template <typename T>
        T getPosY() const { return static_cast<T>(std::floor(y)); }

        float getWidthF() const { return static_cast<float>(getWidth()); }
        float getHeightF() const { return static_cast<float>(getHeight()); }
        
        void destroy() { mIsActive = false; }
        bool isActive() const { return mIsActive; }
        void setActive(bool active) { mIsActive = active; }

        virtual const AABB& getCollisionAABB() const { return mCollisionAABB; } // relative to GameObject x, y position
        virtual const AABB& getObstacleAABB() const { return mObstacleAABB; } // relative to GameObject x, y position
        
        bool checkCollisionAABB(const GameObject& other) const {
            return (x < other.x + other.getWidthF() &&
                    x + getWidthF() > other.x &&
                    y < other.y + other.getHeightF() &&
                    y + getHeightF() > other.y);
        }

        virtual void setPos(float _x, float _y) { 
            x = _x; 
            y = _y; 
        }

    protected:
        // Float positioning for smooth movement
        float x; 
        float y;
        
        bool mIsActive;
        
        AABB mCollisionAABB; // AABB to check collisions. relative to x y coords
        AABB mObstacleAABB; // AABB to check collisions agains map. relative to x y coords
};

class Weapon : public GameObject {
    public:
        enum class OwnerType { Player, Enemy };

        Weapon(float x, float y, OwnerType whoFired, int16_t dmg)
            : GameObject(x, y), mOwner(whoFired), mDamage(dmg) {}

        OwnerType getOwner() const { return mOwner; }
        int16_t getDamage() const { return mDamage; }

    protected:
        OwnerType mOwner;
        int16_t mDamage;

};

class Character : public GameObject {
    public:
        static constexpr int16_t kMaxDamage = std::numeric_limits<int16_t>::max();
        Character(float x, float y, int16_t hp)
            : GameObject(x, y), mHealth(hp), mMaxHealth(hp), mInvincibilityFrames(0) {}

        virtual void update(float dt, ObjectSpawner& spawner) override {
            while (!mFiredWeapons.empty()) {
                spawner.spawnObject(std::move(mFiredWeapons.back()));
                mFiredWeapons.pop_back(); // Shrinks the size of the vector one by one
            }

            assert(mFiredWeapons.empty());
        }

        virtual void takeDamage(int16_t amount, bool force = false) {
            if (mInvincibilityFrames == 0 || force) {
                mHealth -= amount;
                if (mHealth <= 0) {
                    mHealth = 0;
                }
                mInvincibilityFrames = 15; // Flash for 15 frames
            }
        }

    protected:
        int16_t mHealth;
        int16_t mMaxHealth;
        uint8_t mInvincibilityFrames; // To handle flashing when hit

        std::vector<std::unique_ptr<Weapon>> mFiredWeapons;

};

class Arrow: public Weapon {
    public:
        Arrow(float x, float y, float dx, float dy, OwnerType owner): Weapon(x, y, owner, 1), mDx(dx), mDy(dy), 
            mDirection(getDirectionFromMovement(dx, dy)) 
        {
            setSpriteOffsetAndAABBFromDirection(dx, dy);
        }

        void update(float dt, ObjectSpawner& spawner) override final {
            x += mDx * dt;
            y += mDy * dt;
        }

        void draw(SpriteList& sprites) const override final {
            sprites.push({x - mPivotX, y - mPivotY, 64, 1});
        }

        [[nodiscard]] virtual EntityType getType() const override final { return EntityType::Projectile; }

    private:
        void setSpriteOffsetAndAABBFromDirection(float dx, float dy) {
            switch(mDirection) {
                case Direction::UP:
                    mPivotX = 2.0f;
                    mPivotY = 12.0f;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 5, 13);
                    break;
                case Direction::DOWN:
                    mPivotX = 2.0f;
                    mPivotY = 0.0f;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 5, 13);
                    break; 
                case Direction::LEFT:
                    mPivotX = 12.0f;
                    mPivotY = 2.0f;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 13, 5);
                    break;
                case Direction::RIGHT:
                    mPivotX = 0.0f;
                    mPivotY = 2.0f;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 13, 5);
                    break;
                case Direction::LEFT_UP:
                    mPivotX = 10.0f;
                    mPivotY = 10.0f;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 11, 11);
                    break;
                case Direction::LEFT_DOWN:
                    mPivotX = 10.0f;
                    mPivotY = 0.0f;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 11, 11);
                    break; 
                case Direction::RIGHT_DOWN:
                    mPivotX = 0.0f;
                    mPivotY = 0.0f;
                    mCollisionAABB.set(-mPivotX,-mPivotY, 11, 11);
                    break;
                case Direction::RIGHT_UP:
                    mPivotX = 0.0f;
                    mPivotY = 10.0f;
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
};

class Player : public Character {
    public:
        Player(float x, float y) : Character(x, y, 100), mPlayerColor(7), mCurrentWeaponType(0), mScore(0),
            mWalkLegsOscillator(34.0f, 0, 1, 0.5 /* duty cycle */), 
            mWalkJumpOscillator(40.0f, 0.0f, 1.0f, 0.2 /* duty cycle */),
            mFiringCooldownTimer(14.0f /* 14 frames */),
            mDyingAnimationTimer(20.0f /* 20 animation frames */)
        {
            mObstacleAABB = {4, 8, 8, 8}; // lower body half, no hands
        }

        void update(float dt, ObjectSpawner& spawner) override final {
            if(mHealth <= 0) {
                if(!mIsDying) {
                    mIsDying = true;
                    mDyingAnimationTimer.reset();
                } else {
                    if(mDyingAnimationTimer.hasExpired()) setActive(false);
                }
            }

            Character::update(dt, spawner);

            mWalkLegsOscillator.update(dt);
            mWalkJumpOscillator.update(dt);
            mFiringCooldownTimer.update(dt);
            mDyingAnimationTimer.update(dt * 0.2f /* 1/5 fps animation */);

            if (mInvincibilityFrames > 0) {
                mInvincibilityFrames--;
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
            float yy = y - mWalkJumpOscillator.getValue();
            sprites.push({x, yy, legsFlip ? 8 : 16, 1}); // outline
            sprites.push({x, yy, legsFlip ? 4 : 12, mPlayerColor}); // fill

            sprites.push({x, yy - 11, 0, 15}); //horns
        }

        void fire() {
            if(!mCanShoot) return;
            mFiredWeapons.push_back(std::make_unique<Arrow>(x + 8.0f, y, 0.0f, -4.0f, Weapon::OwnerType::Player));
            mCanShoot = false;
            mFiringCooldownTimer.reset();
        }

        [[nodiscard]] bool isDying() const { return mIsDying; }

    private:
        void drawDyingAnimation(SpriteList& sprites) const {
            int timeElapsed = static_cast<int>(mDyingAnimationTimer.getTimeElapsed());

            if(timeElapsed < 15) {
                // flashing anim
                sprites.push({x, y, 8, 1}); // outline
                sprites.push({x, y, 4, timeElapsed % 2 ? mPlayerColor : 15 }); // fill
                sprites.push({x, y-11, 0, 15}); //horns
            } else {
                // burning anim
                sprites.push({x, y, 40, 1}); // ground shadow
                if(timeElapsed < 19) {
                    sprites.push({x, y, 24 + (timeElapsed - 15) * 4, 15}); // fire
                }
            }
        }


    private:
        uint8_t mPlayerColor;
        uint8_t mCurrentWeaponType;
        uint32_t mScore;

        SquareWaveOscillator<float, uint8_t> mWalkLegsOscillator;
        SquareWaveOscillator<float, float> mWalkJumpOscillator;

        GameEngine::Timer mFiringCooldownTimer;
        GameEngine::Timer mDyingAnimationTimer;

        bool mCanShoot = true; // prevent repeated input triggers
        bool mIsDying = false;
};

}  // namespace KnightGame

#endif  // __RETRO_CORE_TEST_GAME_OBJECTS_H

