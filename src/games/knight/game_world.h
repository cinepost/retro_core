#ifndef __RETRO_CORE_GAMES_KNIGHTMARE_GAME_WORLD_H
#define __RETRO_CORE_GAMES_KNIGHTMARE_GAME_WORLD_H

#include "game_objects.h"
#include "bosses.h"
#include "ai_player.h"
#include "sound_files.h"

#include "framework/tmx/tmx_loader.h"
#include "framework/game_engine/sound_player.h"
#include "framework/oscillators.h"
#include "framework/math.h"

#include <array>

namespace KnightGame {

class GameWorld : public ObjectSpawner {
    public: 
        using SoundPlayer = GameEngine::SoundPlayer;

        static const uint16_t kTileSize = 8;
        static const uint16_t kExtraTileSize = 16;

        static const uint16_t kMapWidth = 64;
        static const uint16_t kMapHeight = 256;

        static const uint16_t kExtrasMapWidth = 32;
        static const uint16_t kExtrasMapHeight = 128;

        static constexpr uint16_t kMapTilesCount = kMapWidth * kMapHeight;
        static constexpr uint16_t kMapExtrasCount = kExtrasMapWidth * kExtrasMapHeight;

        class PlayerStatus {
            int8_t   lives = 3;
            uint32_t score = 0;
            uint32_t hiscore = 0;
        };

        class Camera {
            public:
                static const int kViewportWidth = 512;
                static const int kViewportHeight = 272; // 16 pix at the bottom are status line

                static constexpr int kViewportMaxX = kViewportWidth - 1;
                static constexpr int kViewportMaxY = kViewportHeight - 1;

                enum class Direction {
                    UP,
                    DOWN,
                    NONE
                };

                Camera(): mPosY(0), mDirection(Direction::UP), mMoveOscillator(5, 0, 1, 1, 4), mIsPaused(false) {
                }
                
                Camera(uint height): Camera() {
                    setHeight(height);
                }
                
                Camera(uint height, int pos_y): Camera(height) {
                    setPosY(pos_y);
                }

                void setPosY(int pos) {
                    mPosY = std::max(0, pos);
                }

                void stop() {
                    mDirection = Direction::NONE;
                    mMoveOscillator.setRange(0, 0);
                }

                void setPaused(bool state) {
                    mIsPaused = state;
                }

                bool isPaused() const { return mIsPaused; }

                void setCameraSpeed(float s) {
                    if(s < 0.0001f) {
                        mDirection = Direction::NONE;
                        return;
                    }
                    mMoveOscillator.setPeriod(s);
                    mMoveOscillator.setPulsePosition(s-1); 
                }

                void setHeight(uint height) {
                    assert(height > 0);
                    mHeight = height;
                    setPosY(mPosY);
                }

                void setDirection(Direction dir) {
                    if(mDirection == dir) return;
                    mDirection = dir;
                    mMoveOscillator.reset();
                }

                void update(float dt) {
                    mMoveOscillator.update(dt);
                    if(mDirection != Direction::NONE && !mIsPaused) {
                        int offset = mMoveOscillator.getValue();
                        setPosY(getPosY() + (mDirection == Direction::DOWN ? offset : -offset));
                    }
                }

                constexpr int getWidth() const { return 512; /* game hardcoded */ }
                [[nodiscard]] uint getHeight() const { return mHeight; }
                [[nodiscard]] int getPosY() const { return mPosY; }

                template<typename T>
                [[nodiscard]] T getPosY() const { return static_cast<T>(mPosY); }

                [[nodiscard]] uint16_t getSubtilePosY() const {
                    static constexpr int sTileSize = static_cast<int>(GameWorld::kTileSize);
                    return static_cast<uint16_t>(mPosY % sTileSize);
                }

            private:
                int  mPosY ;       // vertical position in pixels (top side)
                Direction mDirection;
                PulseOscillator<float, uint> mMoveOscillator; // drives camera movement

                uint mHeight;      // height in 8x8px tiles
                bool mIsPaused;
        };

        struct Tile {
            enum class Flags: uint8_t {
                None     = 0x00,
                Obstacle = 0x01,
                Water    = 0x02
            };

            DEFINE_ENUM_FLAG_OPERATORS(Flags)

            uint16_t    tile_index; // vdp tile index
            Flags       flags;      // tile flags
        
            Tile(): tile_index(0), flags(Flags::None) {}
            
            Tile(uint16_t _tile_index, Flags _flags = Flags::None): tile_index(_tile_index), flags(_flags) { }

            inline static bool is_set(Flags flag, Flags mask) {
                return (static_cast<uint8_t>(mask) & static_cast<uint8_t>(flag)) == static_cast<uint8_t>(flag);
            }

            void reset() noexcept { tile_index = 0; flags = Flags::None; }

            bool isPassable() const noexcept { return flags == Flags::None; }
        };

        struct Extra {
            enum class State: uint8_t {
                Hidden  = 0,
                Unknown = 1,
                Visible = 2,
                Taken   = 3
            };

            enum class Type: uint8_t {
                EMPTY       = 0,
                POINTS500   = 1, // 500 points
                FREEZE10    = 2, // 10 seconds freeze
                EXTRALIFE   = 3,
                BARRIER     = 4,
                KILLALLSCR  = 5, // Kill all enemies on screen
                EXIT        = 6
            };

            State       state = State::Hidden;
            Type        type  = Type::EMPTY;
            int16_t     hp    = 5;

            Extra() = default;
            Extra(Type _type, State _state): state(_state), type(_type) {};

            void reset() noexcept { type = Type::EMPTY; }

            static Type typeFromString(const std::string& str) {
                if(str == "500") return Type::POINTS500;
                else if(str == "killall") return Type::KILLALLSCR;
                else if(str == "freeze") return Type::FREEZE10;
                else if(str == "barrier") return Type::BARRIER;
                else if(str == "life") return Type::EXTRALIFE;
                else if(str == "exit") return Type::EXIT;
            
                return Type::EMPTY;
            }

            [[nodiscard]] Type getType() const { return type; }

            [[nodiscard]] int16_t getHealth() const { return hp; }

            [[nodiscard]] bool isPassable() const { return !(type == Type::BARRIER && state == State::Visible); }

            // updates extra tile state.
            [[nodiscard]] bool takeDamage(int16_t damage, const AssetManager& am, const SoundPlayer& sp) noexcept {
                if(type == Type::EMPTY || hp == 0 || damage == 0) return false;
                
                static const MP3Stream extraHitSfxTrack(am.getAsset(kExtraBlockHitSfxFileName), false /* no loop */);

                assert(damage > 0);
                switch(state) {
                    case State::Hidden:
                        hp -= 1;
                        state = State::Unknown;
                        sp.playSFX(extraHitSfxTrack);
                        return true;
                    case State::Unknown:
                        hp -= std::min(hp, damage);
                        state = (hp == 0) ? State::Visible : state;
                        sp.playSFX(extraHitSfxTrack);
                        return true;
                    default:
                        return false;
                }
            }

            [[nodiscard]] bool take(const AssetManager& am, const SoundPlayer& sp) noexcept {
                assert(type != Type::EMPTY);

                if(state == State::Visible) {
                    static const MP3Stream extraTakeSfxTrack(am.getAsset(kExtraBlockTakeSfxFileName), false /* no loop */);
                    sp.playSFX(extraTakeSfxTrack);
                    state = State::Taken;
                    hp = 0;
                    return true;
                }
                return false;
            }

            [[nodiscard]] uint16_t getSubtilePatternIndex(uint16_t x, uint16_t y) const noexcept {
                if(type == Type::EMPTY) return 0;

                x = x & 1; y = y & 1;

                uint16_t tile_index_offset;

                switch(state) {
                    case State::Hidden:
                        return 0;
                    case State::Unknown:
                        tile_index_offset = 68;
                        break;
                    case State::Visible:
                        switch(type) {
                            case Type::POINTS500:
                                tile_index_offset = 76;
                                break;
                            case Type::FREEZE10:
                                tile_index_offset = 84;
                                break;
                            case Type::EXTRALIFE:
                                tile_index_offset = 72;
                                break;
                            case Type::KILLALLSCR:
                                tile_index_offset = 80;
                                break;
                            case Type::BARRIER:
                                tile_index_offset = 60;
                                break;
                            case Type::EXIT:
                                tile_index_offset = 88;
                                break;
                            default:
                                assert(false && "Unknown Extra::Type");
                                break;
                        }
                        break;
                    case State::Taken:
                    default:
                        tile_index_offset = 64;
                }

                return tile_index_offset + (uint16_t)(x | (y << 1));
            }
        };

        GameWorld(const V99x8& ppu, const AssetManager& assetManager, const SoundEngine& soundEngine);

        bool init(const std::string& mapFilePath);

        void getContext(WorldContext& ctx) const;

        void clear() { 
            for(auto& tile: mTiles) { tile.reset(); } 
            for(auto& extra_tile: mExtraTiles) { extra_tile.reset(); } 
        }

        void addBoss(std::unique_ptr<Boss> pBossObject) {
            assert(pBossObject);
            mpBoss = pBossObject.get(); 
            mGameObjects.push_back(std::move(pBossObject));

        }

        // World map height in 8x8 tiles
        [[nodiscard]] uint16_t getMapHeight() const noexcept { return mMapHeight; }

        [[nodiscard]] const std::vector<Tile>& getTiles() const noexcept { return mTiles; }

        [[nodiscard]] const Tile& getBackgroundTile(size_t tile_index) const {
            assert(tile_index < mTiles.size()); 
            return mTiles[tile_index]; 
        }

        // get extra tile using 8x8 world coordinates
        [[nodiscard]] const Extra& getExtraTile(uint x, uint y) const {
            uint32_t extra_tile_index = (x >> 1) + ((y >> 1) << 5);
            assert(extra_tile_index < mExtraTiles.size());
            return mExtraTiles[extra_tile_index];
        }

        [[nodiscard]] bool isTilePassable(uint x, uint y) const {
            // check extra 16x16 tiles first
            if(!getExtraTile(x, y).isPassable()) return false;

            uint tile_index = x + y * mMapWidth;
            assert(tile_index < mTiles.size());
            return mTiles[tile_index].isPassable();
        }

        [[nodiscard]] uint16_t getTilePatternIndex(uint tile_index) const { 
            uint tile_y = tile_index >> 6;
            uint tile_x = tile_index % mMapWidth; 
            uint16_t extra_tile = getExtraTile(tile_x, tile_y).getSubtilePatternIndex(tile_x % 2, tile_y % 2);

            return extra_tile == 0 ? mTiles[tile_index].tile_index : extra_tile; 
        }

        [[nodiscard]] const std::vector<GameObject*>& getActiveGameObjects() const noexcept { return mActiveGameObjects; }

        // tests aabb collision with world map and calculates offset to avoid collision
        bool testMapCollisionAt(const GameObject& obj, int offset_x = 0, int offset_y = 0) const {
            const AABB& aabb = obj.getObstacleAABB();
            if(aabb.isSingular()) return false;

            int x_min = obj.getPosX() + offset_x + aabb.x;
            int y_min = obj.getPosY() + offset_y + aabb.y;
            int x_max = x_min + aabb.w - 1;
            int y_max = y_min + aabb.h - 1;

            static const int sTileSize = static_cast<int>(kTileSize);

            int minTileX = std::max(0, x_min / sTileSize);
            int maxTileX = std::min(x_max / sTileSize, static_cast<int>(mMapWidth - 1));
            int minTileY = std::max(0, y_min / sTileSize);
            int maxTileY = std::min(y_max / sTileSize, static_cast<int>(mMapHeight - 1));

            for (int tx = minTileX; tx <= maxTileX; ++tx) {
                for (int ty = minTileY; ty <= maxTileY; ++ty) {

                    //mPPU.drawDebugRect(tx * 8, ty * 8 - mCamera.getPosY(), 8, 8, true);

                    if (!isTilePassable(tx, ty)) {
                        return true; // Collision found!
                    }
                }
            }
            return false;
        }

        void playerRedeemExtra(Player* pPlayer) {
            if(!pPlayer || pPlayer->isDying() || !pPlayer->isActive()) return;

            const AABB& aabb = pPlayer->getObstacleAABB();
            assert(!aabb.isSingular());

            int x_min = pPlayer->getPosX() + aabb.x;
            int y_min = pPlayer->getPosY() + aabb.y;
            int x_max = x_min + aabb.w - 1;
            int y_max = y_min + aabb.h - 1;

            int minTileX = std::max(0, x_min / kExtraTileSize);
            int maxTileX = std::min(x_max / kExtraTileSize, (mMapWidth >> 1) - 1);
            int minTileY = std::max(0, y_min / kExtraTileSize);
            int maxTileY = std::min(y_max / kExtraTileSize, (mMapHeight >> 1) - 1);

            for (int tx = minTileX; tx <= maxTileX; ++tx) {
                for (int ty = minTileY; ty <= maxTileY; ++ty) { 
                    uint32_t extra_tile_index = tx + ty * 32;
                    assert(extra_tile_index < mExtraTiles.size());
                    Extra& extra = mExtraTiles[tx + ty * 32];
                    Extra::Type e_type = extra.getType();

                    if(e_type == Extra::Type::EMPTY) continue;
                    if(extra.take(mAssetManager, mSoundPlayer)) {
                        switch(e_type) {
                            case Extra::Type::POINTS500:
                                pPlayer->adjustScore(500);
                                break;
                            case Extra::Type::EXTRALIFE:
                                pPlayer->adjustLives(1);
                                break;
                            case Extra::Type::KILLALLSCR:
                                killAllEnemiesOnScreen();
                                break;
                            case Extra::Type::FREEZE10:
                                freezeWorld(20);
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
        }

        void playerRedeemUpgrade(Player* pPlayer, Upgrade& upgrade) {
            if(!pPlayer) return;

            assert(upgrade.getType() == GameObject::EntityType::PowerUp || upgrade.getType() == GameObject::EntityType::WeaponUp);
            if(!pPlayer->checkCollisionAABB(upgrade)) return;
            pPlayer->adjustScore(upgrade.getRewardPoints());

            if(upgrade.getType() == GameObject::EntityType::PowerUp) {
                // PowerUp
                PowerUp* pObj = reinterpret_cast<PowerUp*>(&upgrade);
                switch(pObj->getState()) {
                    case PowerUp::State::INVALNURABILE_200:
                        pPlayer->setState(Player::State::INVALNURABLE);
                        break;
                    case PowerUp::State::INVISIBLE_200:
                        pPlayer->setState(Player::State::INVISIBLE);
                        break;
                    case PowerUp::State::SHIELD_200:
                        pPlayer->giveShield();
                        break;
                    default:
                        break;
                }
            } else {
                // WeaponUp
                WeaponUp* pObj = reinterpret_cast<WeaponUp*>(&upgrade);
                pPlayer->setWeaponType(pObj->getPlayerWeaponType());
            }

            upgrade.setActive(false);
        }

        void killAllEnemiesOnScreen() {
            for(GameObject* pObject: mActiveGameObjects) {
                assert(pObject);
                if(pObject->getType() != GameObject::EntityType::Enemy) continue;
                if(isObjectWithinCameraView(pObject)) {
                    reinterpret_cast<Enemy*>(pObject)->takeDamage(1000, mAssetManager, mSoundPlayer, true /* force kill */);
                }
            }
        }

        // Check if object is fully or partially visible within camera view with optional margins
        [[nodiscard]] bool isObjectWithinCameraView(const GameObject* pObject, int margin_x = 0, int margin_y = 0) const {
            assert(pObject);
            const AABB& aabb = pObject->getCollisionAABB();
            int x_min = pObject->getPosX() + aabb.x;
            if(x_min >= (mCamera.kViewportWidth + margin_x)) return false;

            int y_min = pObject->getPosY() + aabb.y;
            if(y_min >= (mCamera.getPosY() + mCamera.kViewportHeight + margin_y)) return false;
            
            int x_max = x_min + aabb.w - 1;
            if(x_max < (0 - margin_x)) return false;
            
            int y_max = y_min + aabb.h - 1;
            return !(y_max < (mCamera.getPosY() - margin_y));
        }

        // special case when we check for enemies to spawn and/or update. Only spawn enemies that are close enough to camera's upper edge
        [[nodiscard]] bool isObjectLowerEdgeWithinCameraView(const GameObject* pObject, int margin = 0) const {
            assert(pObject);
            const AABB& aabb = pObject->getCollisionAABB();
            int y_max = pObject->getPosY() + aabb.y + aabb.h - 1;
            return !(y_max < (mCamera.getPosY() - margin));
        }

        void freezeWorld(unsigned int seconds) {
            mCamera.setPaused(true);
            mFreezeTimer.start(static_cast<float>(seconds * 60), [&]() { mCamera.setPaused(false); });
        }

        [[nodiscard]] bool isWorldFrozen() const {
            return !mFreezeTimer.hasExpired();
        }

        void processPlayerEnemyCollision(Player* pPlayer, Enemy& enemy) {
            if(!pPlayer || !pPlayer->isActive() || pPlayer->isDying() || pPlayer->getState() == Player::State::INVISIBLE) return;

            if(!enemy.isActive() || !pPlayer->checkCollisionAABB(enemy)) return;

            enemy.takeDamage(1000, mAssetManager, mSoundPlayer, true /*force kill */);
            if(pPlayer->getState() != Player::State::INVALNURABLE) {
                pPlayer->takeDamage(1000, mAssetManager, mSoundPlayer, true /* force kill */);
            }
        }

        void processEnemyWeaponCollisions(Weapon& weapon) {
            assert(weapon.getOwner() != Weapon::OwnerType::Player);
        }

        void processPlayerWeaponCollisions(Weapon& weapon) {
            assert(weapon.getOwner() == Weapon::OwnerType::Player);

            if(!weapon.isActive()) return;
            
            const AABB& aabb = weapon.getCollisionAABB();
            int weaponMinX = weapon.getPosX() + aabb.x;
            int weaponMinY = weapon.getPosY() + aabb.y;
            int weaponMaxX = weaponMinX + aabb.w - 1;
            int weaponMaxY = weaponMinY + aabb.h - 1;

            // first test against boss, enemies and upgrades
            for(GameObject* pObject: mActiveGameObjects) {
                assert(pObject);
                const GameObject::EntityType objType = pObject->getType();

                switch(objType) {
                    case GameObject::EntityType::PowerUp:
                    case GameObject::EntityType::WeaponUp:
                        if(weapon.checkCollisionAABB(*pObject)) {
                            reinterpret_cast<Upgrade*>(pObject)->toggleState(mAssetManager, mSoundPlayer);
                            weapon.setActive(false);
                            return; // weapon wasted. return
                        }
                        break;
                    case GameObject::EntityType::Enemy:
                        {
                            Enemy* pEnemy = reinterpret_cast<Enemy*>(pObject);
                            if(weapon.checkCollisionAABB(*pObject)) {
                                int16_t enemy_prev_health = pEnemy->getHealth();
                                if(pEnemy->takeDamage(weapon.getDamage(), mAssetManager, mSoundPlayer)) {
                                    weapon.decreaseDamage(std::max(0, enemy_prev_health - pEnemy->getHealth()));
                                    if(weapon.getDamage() <= 0) {
                                        weapon.setActive(false);
                                        return; // weapon wasted. return
                                    }
                                }  
                            }
                        }
                        break;
                    default:
                        break;
                }
            }

            if(!weapon.isActive() || weapon.getDamage() <= 0) return;

            // test against extra tiles
            static const int sExtraTileSize = static_cast<uint>(kExtraTileSize);

            int minTileX = std::max(0, weaponMinX / sExtraTileSize);
            int maxTileX = std::min(weaponMaxX / sExtraTileSize, static_cast<int>(kExtrasMapWidth - 1));
            int minTileY = std::max(0, weaponMinY / sExtraTileSize);
            int maxTileY = std::min(weaponMaxY / sExtraTileSize, static_cast<int>(kExtrasMapHeight - 1));

            for (int tx = minTileX; tx <= maxTileX; ++tx) {
                for (int ty = minTileY; ty <= maxTileY; ++ty) {

                    //mPPU.drawDebugRect(tx * 16, ty * 16 - mCamera.getPosY(), 16, 16, false);

                    uint32_t extra_tile_index = tx + ty * 32;
                    assert(extra_tile_index < mExtraTiles.size());

                    Extra& extra = mExtraTiles[extra_tile_index];
                    int16_t extra_prev_health = extra.getHealth();
                    if(extra.takeDamage(weapon.getDamage(), mAssetManager, mSoundPlayer)) {
                        // adjust weapon damage
                        weapon.setActive(false);
                        break;
                    }
                }
            }
        }

        void moveObject(GameObject& obj, int dx, int dy) {
            int maxMove = std::max(std::abs(dx), std::abs(dy));
    
            // Let's use 7.0f to be safe, guaranteeing we never skip over an 8px tile.
            static const float MAX_STEP_SIZE = 7.0f; 
            static const float sTileSizeF = static_cast<float>(kTileSize);
    
            uint16_t numSteps = static_cast<uint16_t>(std::ceil(static_cast<float>(maxMove) / MAX_STEP_SIZE));
            if (numSteps < 1) numSteps = 1;

            float stepX = static_cast<float>(dx) / numSteps;
            float stepY = static_cast<float>(dy) / numSteps;

            float offset_x = 0.0f;
            float offset_y = 0.0f;
            const AABB& aabb = obj.getObstacleAABB();
            const float aabb_w_f = static_cast<float>(aabb.w);
            const float aabb_h_f = static_cast<float>(aabb.h);

            for (uint16_t step = 0; step < numSteps; ++step) {
                
                // X axis first
                offset_x += stepX;
                if (testMapCollisionAt(obj, static_cast<int>(offset_x), static_cast<int>(offset_y))) {
                    if (stepX > 0.0f) {
                        float tileX = std::floor((offset_x + aabb_w_f) / sTileSizeF);
                        offset_x = (tileX - 1) * sTileSizeF;
                    } else if (stepX < 0.0f) {
                        float tileX = std::floor(offset_x / sTileSizeF);
                        offset_x = (tileX + 1) * sTileSizeF;
                    }
                    stepX = 0.0f; // Stop further X movement in remaining substeps
                }

                offset_y += stepY;
                if (testMapCollisionAt(obj, static_cast<int>(offset_x), static_cast<int>(offset_y))) {
                    if (stepY > 0.0f) {
                        float tileY = std::floor((offset_y + aabb_h_f) / sTileSizeF);
                        offset_y = (tileY - 1) * sTileSizeF;
                    } else if (stepY < 0.0f) {
                        float tileY = std::floor(offset_y / sTileSizeF);
                        offset_y = (tileY + 1) * sTileSizeF;
                    }
                    stepY = 0.0f; // Stop further Y movement in remaining substeps
                }
            }

            obj.setPos(obj.getPosX() + static_cast<int>(std::floor(offset_x)), obj.getPosY() + static_cast<int>(std::floor(offset_y)));
        }


        void movePlayer(uint player_id, int dx, int dy) {
            assert(player_id < 2);

            Player* pPlayer = mpPlayers[player_id];
            if(!pPlayer) return;

            static const int sViewportTopGap = 32; // dont let player move closer to camera top edge
            
            // camera bounds with 16px gap on top
            if(!pPlayer->isDying()) {
                moveObject(*pPlayer, dx, dy);
                const AABB& aabb = pPlayer->getObstacleAABB();

                pPlayer->setPos(
                    std::min(mCamera.kViewportMaxX - (aabb.x + aabb.w - 1), std::max(-aabb.x, pPlayer->getPosX())), 
                    std::min(mCamera.getPosY() + mCamera.kViewportMaxY - (aabb.y + aabb.h - 1), std::max(mCamera.getPosY() + sViewportTopGap - aabb.y, pPlayer->getPosY()))
                );
            }

            // check if trapped. then kill player
            if(testMapCollisionAt(*pPlayer)) {
                pPlayer->takeDamage(Character::kMaxDamage, mAssetManager, mSoundPlayer, true /* force kill */);
                
                if(isOnlyOrBothPlayersDying()) {
                    // last player or all players are dead. stop camera
                    mCamera.stop();
                }
            }
        }

        void firePlayer(int player_id) {
            assert(player_id < 2);
            if(!mpPlayers[player_id]) return;
            mpPlayers[player_id]->fire(mAssetManager, mSoundPlayer);
        }

        void spawnObject(std::unique_ptr<GameObject> pNewObject) override final {
            if (!pNewObject) return;
            mPendingObjects.push_back(std::move(pNewObject));
        }

        void update(float dt);

        void wakeUpBoss() {
            assert(mpBoss);
            mpBoss->wakeUp();
        }

        const std::array<Player*, 2>& getPlayers() const { assert(mIsInitialized); return mpPlayers; }

        const Player* getPlayer(uint player_id) const { assert(player_id < 2); return mpPlayers[player_id]; }

        [[nodiscard]] bool isGameOver() const {
            return mpPlayers[0] == nullptr && mpPlayers[1] == nullptr;
        }

        [[nodiscard]] bool isPlayerDead(uint player_id) const {
            assert(player_id < 2);
            return mpPlayers[player_id] == nullptr;
        }

        // Is player dying in sinle player mode or last standing player or both is dying in multiplayer mode
        [[nodiscard]] bool isOnlyOrBothPlayersDying() const {
            if(!mpPlayers[0] && !mpPlayers[1]) return false;
            return  (mpPlayers[0] && mpPlayers[0]->isDying() && mpPlayers[1] == nullptr) || 
                    (mpPlayers[1] && mpPlayers[1]->isDying() && mpPlayers[0] == nullptr) ||
                    (mpPlayers[0] && mpPlayers[0]->isDying() && mpPlayers[1] && mpPlayers[1]->isDying());
        }

        Camera& getCamera() { return mCamera; }
        void setCameraHeight(uint16_t height) { mCamera.setHeight(height); }

        // Offset to camera visible tiles
        uint32_t getCurrentCameraTilesOffset() const {
            return static_cast<uint32_t>((mCamera.getPosY() / kTileSize) * kMapWidth);
        }

    private:
        const V99x8& mPPU; // for debug

        // A non-owning, weak shortcut pointer providing immediate access to the player instance
        std::array<Player*, 2>  mpPlayers; 
        Boss*                   mpBoss = nullptr;

        bool                mIsInitialized = false;
        uint16_t            mMapWidth  = 0;
        uint16_t            mMapHeight = 0;
        const AssetManager& mAssetManager;
        SoundPlayer         mSoundPlayer;
        Camera              mCamera;

        std::vector<Extra>  mExtraTiles; // Tiles that holds extra power ups
        std::vector<Tile>   mTiles;

        std::vector<std::unique_ptr<GameObject>> mGameObjects; 
        std::vector<std::unique_ptr<GameObject>> mPendingObjects; // Deferred spawn list

        std::vector<GameObject*>                 mActiveGameObjects;

        // timers
        GameEngine::Timer mFreezeTimer;

        uint32_t mFrameNumber = 0;
};

}  // namespace KnightGame

#endif  // __RETRO_CORE_GAMES_KNIGHTMARE_GAME_WORLD_H

