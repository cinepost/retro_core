#ifndef __RETRO_CORE_FRAMEWORK_GAME_ENGINE_GAME_OBJECT_H
#define __RETRO_CORE_FRAMEWORK_GAME_ENGINE_GAME_OBJECT_H

namespace RetroCore {

namespace GameEngine {

enum class EntityType { 
    Player, 
    Enemy, 
    PowerUp, 
    Obstacle, 
    Projectile 
};

class GameObject {
    public:
        GameObject(float x, float y, EntityType type) : m_x(x), m_y(y), m_type(type), m_active(true) {}
        virtual ~GameObject() = default;

        virtual void update(double dt) = 0;
        virtual void render() = 0;
        
        virtual void onCollision(GameObject& other) = 0;

        bool isActive() const { return m_active; }
        void destroy() { m_active = false; }
        EntityType getType() const { return m_type; }

    protected:
        float m_x, m_y;
        float m_width = 32.0f, m_height = 32.0f; // Standard bounding box defaults
        EntityType m_type;
        bool m_active;
};

// Base class for anything moving (physics-driven components)
class MovingEntity : public GameObject {
    public:
        using GameObject::GameObject;
        void update(double dt) override {
            m_x += m_vx * dt;
            m_y += m_vy * dt;
        }
    protected:
        float m_vx = 0.0f, m_vy = 0.0f;
        float m_speed = 150.0f;
};

// Player Class
class Player : public MovingEntity {
    public:
        Player(float x, float y) : MovingEntity(x, y, EntityType::Player) {}
        void update(double dt) override {
            // Apply gravity if side-scroller platformer
            m_vy += m_gravity * dt; 
            MovingEntity::update(dt);
        }
        void render() override;
        void onCollision(GameObject& other) override {
            if (other.GetType() == EntityType::Enemy) {
                HandleDamage();
            }
        }
        void handleDamage();
        void givePowerUp(int powerType);

    private:
        bool m_isGrounded = false;
        float m_gravity = 981.0f;
        int m_currentPowerState = 0; // 0 = normal, 1 = super, 2 = fire/invincible
};

// Enemy Subclasses
class Enemy : public MovingEntity {
    public:
        Enemy(float x, float y) : MovingEntity(x, y, EntityType::Enemy) {}
        void onCollision(GameObject& other) override;
};

class SideScrollerPatroller : public Enemy {
    public:
        using Enemy::Enemy;
        void update(double dt) override {
            // Simple horizontal pacing behavior
            if (m_hitWall) {
                m_vx *= -1; 
            }
            MovingEntity::update(dt);
        }
    private:
        bool m_hitWall = false;
};

// Items and Interactables
class PowerUp : public GameObject {
    public:
        PowerUp(float x, float y, int type) 
            : GameObject(x, y, EntityType::PowerUp), m_powerType(type) {}
        
        void update(double dt) override {} // Often static or simple floating animation
        void render() override;
        
        void onCollision(GameObject& other) override {
            if (other.GetType() == EntityType::Player) {
                static_cast<Player&>(other).GivePowerUp(m_powerType);
                destroy(); // Remove from game loop pool
            }
        }
    private:
        int m_powerType;
};


}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_GAME_OBJECT_H
