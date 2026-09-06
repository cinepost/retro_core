#ifndef __RETRO_CORE_TEST_GAME_OBJECT_H
#define __RETRO_CORE_TEST_GAME_OBJECT_H

#include <cstdint>
#include "framework/ppu/ppu_msx.h"

namespace KnightGame {

struct Rect {
    float pos_x;
    float pos_y;
    uint16_t width;
    uint16_t height;
};

class GameObject {
    public:
        GameObject(float start_x, float start_y, uint16_t w, uint16_t h)
            : x(start_x), y(start_y), dx(0), dy(0), width(w), height(h), isActive(true) 
        {}

        virtual ~GameObject() = default;

        // Pure virtual functions every game object must implement
        virtual void update(float deltaTime) = 0;
        virtual void draw(MsxPPU_BASE& ppu) = 0; // Uploads/updates data to your virtual VDP structure
        
        // Inline Getters/Setters for rapid collision checking
        bool isActive() const { return isActive; }
        void setActive(bool active) { isActive = active; }
        
        // Quick Axis-Aligned Bounding Box (AABB) check
        bool checkCollision(const GameObject& other) const {
            return (x < other.x + other.mWidth &&
                    x + mWidth > other.x &&
                    y < other.y + other.mHeight &&
                    y + mHeight > other.y);
        }

    protected:
        // Fixed-point positioning for smooth movement
        float x; 
        float y;
        float dx;
        float dy;

        uint16_t mWidth;
        uint16_t mHeight;

        bool isActive;
};

class Character : public GameObject {
    public:
        Character(float x, float y, uint8_t w, uint8_t h, int16_t hp)
            : GameObject(x, y, w, h), health(hp), maxHealth(hp), invincibilityFrames(0) {}

        virtual void takeDamage(int16_t amount) {
            if (invincibilityFrames == 0) {
                mHealth -= amount;
                if (mHealth <= 0) {
                    mHealth = 0;
                    setActive(false);
                }
                invincibilityFrames = 15; // Flash for 15 frames
            }
        }

    protected:
        int16_t mHealth;
        int16_t mMaxHealth;
        uint8_t mInvincibilityFrames; // To handle flashing when hit

};

class Weapon : public GameObject {
    public:
        enum class OwnerType { Player, Enemy };

        Weapon(float x, float y, uint8_t w, uint8_t h, OwnerType whoFired, int16_t dmg)
            : GameObject(x, y, w, h), owner(whoFired), damage(dmg) {}

        OwnerType getOwner() const { return mOwner; }
        int16_t getDamage() const { return mDamage; }

    protected:
        OwnerType mOwner;
        int16_t mDamage;

};

class Player : public Character {
    public:
        Player(float x, float y) : Character(x, y, 16, 16, 1), mCurrentWeaponType(0), mScore(0) {}

        void update(float deltaTime) override {
            // 1. Read virtual input (Arrow keys / Space)
            // 2. Adjust dx, dy based on input
            // 3. Update x, y positions
            // 4. Handle weapon firing timers
            if (mInvincibilityFrames > 0) mInvincibilityFrames--;
        }

        void draw() override {
            // Map player x, y directly to VDP Sprite Attribute Table Slot 0
        }

    private:
        uint8_t mCurrentWeaponType;
        uint32_t mScore;
};

}  // namespace KnightGame

#endif  // __RETRO_CORE_TEST_GAME_OBJECT_H

