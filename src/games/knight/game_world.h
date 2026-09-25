#ifndef __RETRO_CORE_TEST_GAME_WORLD_H
#define __RETRO_CORE_TEST_GAME_WORLD_H

#include "game_objects.h"

#include "framework/tmx/tmx_loader.h"
#include "framework/game_engine/sound_player.h"
#include "framework/oscillators.h"

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
                enum class Direction {
                    UP,
                    DOWN,
                    NONE
                };

                Camera(): mPosY(0), mDirection(Direction::UP), mMoveOscillator(5, 0, 1, 1, 4) {
                }
                
                Camera(uint16_t height): Camera() {
                    setHeight(height);
                }
                
                Camera(uint16_t height, uint16_t pos_y): Camera(height) {
                    setPosY(pos_y);
                }

                void setPosY(uint16_t pos) {
                    mPosY = pos;
                }

                void stop() {
                    mDirection = Direction::NONE;
                    mMoveOscillator.setRange(0, 0);
                }

                void setCameraSpeed(uint16_t s) {
                    if(s == 0) {
                        mDirection = Direction::NONE;
                        return;
                    }
                    mMoveOscillator.setPeriod(s);
                    mMoveOscillator.setPulsePosition(s-1); 
                }

                void setHeight(uint16_t height) {
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
                    if(mDirection != Direction::NONE) {
                        uint16_t offset = mMoveOscillator.getValue();
                        setPosY(getPosY() + (mDirection == Direction::DOWN ? offset : -offset));
                    }
                }

                uint16_t getHeight() const { return mHeight; }
                uint16_t getPosY() const { return mPosY; }

                uint16_t getSubtilePosY() const {
                    return mPosY % GameWorld::kTileSize;
                }

            private:
                uint16_t  mPosY ;       // vertical position in pixels (top side)
                Direction mDirection;
                PulseOscillator<uint16_t, uint16_t> mMoveOscillator; // drives camera movement

                uint16_t  mHeight;      // height in 8x8px tiles
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

            void reset() noexcept { tile_index = 0; Flags::None; }

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
                
                static const MP3Stream extraHitSfxTrack(am.getAsset("sfx/extra_block_hit.mp3"), false /* no loop */);

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
                    static const MP3Stream extraTakeSfxTrack(am.getAsset("sfx/extra_block_take.mp3"), false /* no loop */);
                    sp.playSFX(extraTakeSfxTrack);
                    state = State::Taken;
                    hp = 0;
                    return true;
                }
                return false;
            }

            [[nodiscard]] uint16_t getSubtilePatternIndex(uint8_t x, uint8_t y) const noexcept {
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

                return tile_index_offset + (x | (y << 1));
            }
        };

        GameWorld(const V99x8& ppu, const AssetManager& assetManager, const SoundEngine& soundEngine);

        bool init(const std::string& mapFilePath);

        void clear() { 
            for(auto& tile: mTiles) { tile.reset(); } 
            for(auto& extra_tile: mExtraTiles) { extra_tile.reset(); } 
        }

        // World map height in 8x8 tiles
        [[nodiscard]] uint16_t getMapHeight() const noexcept { return kMapHeight; }

        [[nodiscard]] const std::vector<Tile>& getTiles() const noexcept { return mTiles; }

        [[nodiscard]] const Tile& getBackgroundTile(uint32_t tile_index) const {
            assert(tile_index < mTiles.size()); 
            return mTiles[tile_index]; 
        }

        // get extra tile using 8x8 world coordinates
        [[nodiscard]] const Extra& getExtraTile(uint16_t x, uint16_t y) const {
            uint32_t extra_tile_index = (x >> 1) + ((y >> 1) << 5);
            assert(extra_tile_index < mExtraTiles.size());
            return mExtraTiles[extra_tile_index];
        }

        [[nodiscard]] bool isTilePassable(uint16_t x, uint16_t y) const {
            // check extra 16x16 tiles first
            if(!getExtraTile(x, y).isPassable()) return false;

            uint16_t tile_index = x + y * kMapWidth;
            assert(tile_index < mTiles.size());
            return mTiles[tile_index].isPassable();
        }

        [[nodiscard]] const uint16_t getTilePatternIndex(uint32_t tile_index) const { 
            uint16_t tile_y = tile_index >> 6;
            uint16_t tile_x = tile_index % kMapWidth; 
            uint16_t extra_tile = getExtraTile(tile_x, tile_y).getSubtilePatternIndex(tile_x % 2, tile_y % 2);

            return extra_tile == 0 ? mTiles[tile_index].tile_index : extra_tile; 
        }

        [[nodiscard]] const std::vector<std::unique_ptr<GameObject>>& getGameObjects() const noexcept { return mGameObjects; }

        // tests aabb collision with world map and calculates offset to avoid collision
        bool testMapCollisionAt(const GameObject& obj, float offset_x = 0.0f, float offset_y = 0.0f) const {
            const AABB& aabb = obj.getObstacleAABB();
            if(aabb.isSingular()) return false;

            float x = obj.getPosX() + offset_x + aabb.x;
            float y = obj.getPosY() + offset_y + aabb.y;

            static const float sTileSizeF = static_cast<float>(kTileSize);

            int16_t minTileX = std::max((int16_t)0, static_cast<int16_t>(std::floor(x / sTileSizeF)));
            int16_t maxTileX = std::min(static_cast<int16_t>(std::floor((x + aabb.w - 0.001f) / sTileSizeF)), static_cast<int16_t>(kMapWidth - 1));
            int16_t minTileY = std::max((int16_t)0, static_cast<int16_t>(std::floor(y / sTileSizeF)));
            int16_t maxTileY = std::min(static_cast<int16_t>(std::floor((y + aabb.h - 0.001f) / sTileSizeF)), static_cast<int16_t>(kMapHeight - 1));

            for (uint16_t tx = minTileX; tx <= maxTileX; ++tx) {
                for (uint16_t ty = minTileY; ty <= maxTileY; ++ty) {
                    if (!isTilePassable(tx, ty)) {
                        return true; // Collision found!
                    }
                }
            }
            return false;
        }

        void playerRedeemExtra(Player& player) {
            const AABB& aabb = player.getObstacleAABB();
            assert(!aabb.isSingular());

            int x_min = player.getPosX<int>() + aabb.x;
            int y_min = player.getPosY<int>() + aabb.y;
            int x_max = x_min + aabb.w;
            int y_max = y_min + aabb.h;

            int minTileX = std::max(0, x_min / kExtraTileSize);
            int maxTileX = std::min(x_max / kExtraTileSize, (mMapWidth >> 1) - 1);
            int minTileY = std::max(0, y_min / kExtraTileSize);
            int maxTileY = std::min(y_max / kExtraTileSize, (mMapHeight >> 1) - 1);

            for (uint16_t tx = minTileX; tx <= maxTileX; ++tx) {
                for (uint16_t ty = minTileY; ty <= maxTileY; ++ty) { 
                    uint32_t extra_tile_index = tx + ty * 32;
                    assert(extra_tile_index < mExtraTiles.size());
                    Extra& extra = mExtraTiles[tx + ty * 32];
                    Extra::Type e_type = extra.getType();

                    if(e_type == Extra::Type::EMPTY) continue;
                    if(extra.take(mAssetManager, mSoundPlayer)) {
                        switch(e_type) {
                            case Extra::Type::POINTS500:
                                player.adjustScore(500);
                                break;
                            case Extra::Type::EXTRALIFE:
                                player.adjustLives(1);
                                break;
                            case Extra::Type::KILLALLSCR:
                                killAllEnemiesOnScreen();
                                break;
                            case Extra::Type::FREEZE10:
                                freezeWorld(10);
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
                }
            } else {
                // WeaponUp
            }

            upgrade.setActive(false);
        }

        void killAllEnemiesOnScreen() {}

        void freezeWorld(unsigned int seconds) {
            mFreezeTimer.start(static_cast<float>(seconds));
        }

        void processPlayerWeaponCollisions(Weapon& weapon) {
            if(weapon.getOwner() != Weapon::OwnerType::Player || !weapon.isActive()) return;
            
            const AABB& aabb = weapon.getCollisionAABB();
            float x = weapon.getPosX() + aabb.x;
            float y = weapon.getPosY() + aabb.y;

            // first test against enemies

            // test against upgrades
            for(std::unique_ptr<GameObject>& pObject: mGameObjects) {
                if(pObject->getType() == GameObject::EntityType::PowerUp || pObject->getType() == GameObject::EntityType::WeaponUp) {
                    if(weapon.checkCollisionAABB(*pObject.get())) {
                        reinterpret_cast<Upgrade*>(pObject.get())->toggleState();
                        weapon.setActive(false);
                        return;
                    }
                }
            }

            // test against weapon upgrades

            // test against extra tiles
            static const float sExtraTileSizeF = static_cast<float>(kExtraTileSize);

            int16_t minTileX = std::max((int16_t)0, static_cast<int16_t>(std::floor(x / sExtraTileSizeF)));
            int16_t maxTileX = std::min(static_cast<int16_t>(std::floor((x + aabb.w - 0.001f) / sExtraTileSizeF)), static_cast<int16_t>(kExtrasMapWidth - 1));
            int16_t minTileY = std::max((int16_t)0, static_cast<int16_t>(std::floor(y / sExtraTileSizeF)));
            int16_t maxTileY = std::min(static_cast<int16_t>(std::floor((y + aabb.h - 0.001f) / sExtraTileSizeF)), static_cast<int16_t>(kExtrasMapHeight - 1));

            for (int16_t tx = minTileX; tx <= maxTileX; ++tx) {
                for (int16_t ty = minTileY; ty <= maxTileY; ++ty) {
                    uint32_t extra_tile_index = tx + ty * 32;
                    assert(extra_tile_index < mExtraTiles.size());

                    Extra& extra = mExtraTiles[extra_tile_index];
                    int16_t extra_prev_health = extra.getHealth();
                    if(extra.takeDamage(weapon.getDamage(), mAssetManager, mSoundPlayer)) {
                        // adjust weapon damage
                        weapon.decreaseDamage(std::min(0, extra_prev_health - extra.getHealth()));
                    }
                }
            }
        }

        void moveObject(GameObject& obj, float dx, float dy) {
            float maxMove = std::max(std::abs(dx), std::abs(dy));
    
            // Let's use 7.0f to be safe, guaranteeing we never skip over an 8px tile.
            static const float MAX_STEP_SIZE = 7.0f; 
            static const float sTileSizeF = static_cast<float>(kTileSize);
    
            uint16_t numSteps = static_cast<uint16_t>(std::ceil(maxMove / MAX_STEP_SIZE));
            if (numSteps < 1) numSteps = 1;

            float stepX = dx / numSteps;
            float stepY = dy / numSteps;

            float offset_x = 0.0f;
            float offset_y = 0.0f;
            const AABB& aabb = obj.getObstacleAABB();

            for (uint16_t step = 0; step < numSteps; ++step) {
                
                // X axis first
                offset_x += stepX;
                if (testMapCollisionAt(obj, offset_x, offset_y)) {
                    if (stepX > 0.0f) {
                        int16_t tileX = static_cast<int16_t>(std::floor((offset_x + aabb.w) / sTileSizeF));
                        offset_x = static_cast<float>((tileX - 1) * sTileSizeF);
                    } else if (stepX < 0.0f) {
                        int16_t tileX = static_cast<int16_t>(std::floor(offset_x / sTileSizeF));
                        offset_x = static_cast<float>((tileX + 1) * sTileSizeF);
                    }
                    stepX = 0.0f; // Stop further X movement in remaining substeps
                }

                offset_y += stepY;
                if (testMapCollisionAt(obj, offset_x, offset_y)) {
                    if (stepY > 0.0f) {
                        int16_t tileY = static_cast<int16_t>(std::floor((offset_y + aabb.h) / sTileSizeF));
                        offset_y = static_cast<float>((tileY - 1) * sTileSizeF);
                    } else if (stepY < 0.0f) {
                        int16_t tileY = static_cast<int16_t>(std::floor(offset_y / sTileSizeF));
                        offset_y = static_cast<float>((tileY + 1) * sTileSizeF);
                    }
                    stepY = 0.0f; // Stop further Y movement in remaining substeps
                }
            }

            obj.setPos(obj.getPosX() + offset_x, obj.getPosY() + offset_y);
        }


        void movePlayer(uint player_id, float dx, float dy) {
            assert(player_id < 2);

            Player* pPlayer = mpPlayers[player_id];
            if(!pPlayer) return;

            static const float sViewportTopGap = 32.0f;
            static const float sViewportMaxX = 511.0f;
            static const float sViewportMaxY = 272.0f; // 16 pix at the bottom are status line

            const AABB& aabb = pPlayer->getObstacleAABB();
            
            moveObject(*pPlayer, dx, dy);
            
            // camera bounds with 16px gap on top
            if(!pPlayer->isDying()) {
                pPlayer->setPos(
                    std::min(sViewportMaxX - (aabb.x + aabb.w), std::max(0.0f - aabb.x, pPlayer->getPosX())), 
                    std::min((float)mCamera.getPosY() + sViewportMaxY - (aabb.y + aabb.h), std::max((float)mCamera.getPosY() + sViewportTopGap - aabb.y, pPlayer->getPosY()))
                );
            }

            // check if trapped. then kill player
            if(testMapCollisionAt(*pPlayer)) {
                pPlayer->takeDamage(Character::kMaxDamage, true /* force kill */);
                
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
            static constexpr float sTileHeightF = static_cast<float>(kTileSize);
            return static_cast<uint32_t>(std::floor(static_cast<float>(mCamera.getPosY()) / sTileHeightF)) * kMapWidth;
        }

    private:
        const V99x8& mPPU; // for debug

        // A non-owning, weak shortcut pointer providing immediate access to the player instance
        std::array<Player*, 2> mpPlayers; 

        bool                mIsInitialized = false;
        uint16_t            mMapWidth  = 0;
        uint16_t            mMapHeight = 0;
        const AssetManager& mAssetManager;
        SoundPlayer         mSoundPlayer;
        Camera              mCamera;

        std::vector<Extra> mExtraTiles; // Tiles that holds extra power ups
        std::vector<Tile> mTiles;

        std::vector<std::unique_ptr<GameObject>> mGameObjects; 
        std::vector<std::unique_ptr<GameObject>> mPendingObjects; // Deferred spawn list

        // timers
        GameEngine::Timer mFreezeTimer;

        uint32_t mFrameNumber = 0;
};

}  // namespace KnightGame

#endif  // __RETRO_CORE_TEST_GAME_WORLD_H

