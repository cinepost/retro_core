#ifndef __RETRO_CORE_TEST_GAME_WORLD_H
#define __RETRO_CORE_TEST_GAME_WORLD_H

#include "game_objects.h"

#include "framework/tmx/tmx_loader.h"
#include "framework/oscillators.h"

namespace KnightGame {

class GameWorld : public ObjectSpawner {
    public: 
        static const uint16_t kTileSize = 8;

        static const uint16_t kMapWidth = 64;
        static const uint16_t kMapHeight = 256;

        static const uint16_t kExtrasMapWidth = 32;
        static const uint16_t kExtrasMapHeight = 128;

        static constexpr uint16_t kMapTilesCount = kMapWidth * kMapHeight;
        static constexpr uint16_t kMapExtrasCount = kExtrasMapWidth * kExtrasMapHeight;

        class SoundPlayer {
            public:
                SoundPlayer(const SoundEngine& soundEngine): mSoundEngine(soundEngine) {}

                void playSFX(std::unique_ptr<GameEngine::AudioSource> sfxTrack) const {
                    mSoundEngine.playSFX(std::move(sfxTrack));
                }

            private:
                const SoundEngine& mSoundEngine;
        };

        class Camera {
            public:
                enum class Direction {
                    UP,
                    DOWN,
                    NONE
                };

                Camera(uint16_t map_height): mPosY(0), mMapHeight(map_height), mDirection(Direction::UP), 
                    mPulseOscillator(5, 0, 1, 1, 4) /* 1 scanline in 5 frames */
                {
                    
                    assert(mMapHeight > 0);
                }
                
                Camera(uint16_t height, uint16_t map_height): Camera(map_height) {
                    setHeight(height);
                }
                
                Camera(uint16_t height, uint16_t map_height, uint16_t pos_y): Camera(height, map_height) {
                    setPosY(pos_y);
                }

                void setPosY(uint16_t pos) {
                    if(pos > mMaxPosY) {
                        mPosY = mMaxPosY;
                        return;
                    }
                    mPosY = pos;
                }

                void stop() {
                    mDirection = Direction::NONE;
                    mPulseOscillator.setRange(0, 0);
                }

                void setCameraSpeed(uint16_t s) {
                    if(s == 0) {
                        mDirection = Direction::NONE;
                        return;
                    }
                    mPulseOscillator.setPeriod(s);
                    mPulseOscillator.setPulsePosition(s-1); 
                }

                void setHeight(uint16_t height) {
                    assert(height > 0);
                    mHeight = height;
                    mMaxPosY = mMapHeight * GameWorld::kTileSize - mHeight * GameWorld::kTileSize;
                    setPosY(mPosY);
                }

                void setDirection(Direction dir) {
                    if(mDirection == dir) return;
                    mDirection = dir;
                    mPulseOscillator.reset();
                }

                void update(float dt) {
                    mPulseOscillator.update(1);  // frame based
                    if(mDirection != Direction::NONE) {
                        uint16_t offset = mPulseOscillator.getValue();
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
                uint16_t  mMapHeight;
                Direction mDirection;
                PulseOscillator<uint16_t, uint16_t> mPulseOscillator; // drives camera movement

                uint16_t  mHeight;      // height in 8x8px tiles
                uint16_t  mMaxPosY;
        };

        struct Tile {
            enum class Type: uint8_t {
                None    = 0,
                Ground  = 1,
                Wall    = 2,
                Water   = 3,
                Bridge  = 4,
            };

            enum class Flags: uint8_t {
                None    = 0x00,
            };

            uint16_t    tile_index; // vdp tile index
            Type        type;  
            Flags       flags;      // tile flags
        
            Tile(): tile_index(0), type(Type::None), flags(Flags::None) {}
            Tile(uint16_t _tile_index, Type _type, Flags _flags = Flags::None): tile_index(_tile_index), type(_type) {
                flags = Flags((uint8_t)flags | (uint8_t)_flags);
            }

            void reset() noexcept { tile_index = 0; type = Type::None; flags = Flags::None; }

            bool isPassable() const noexcept { return type != Type::Wall && type != Type::Water; }
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

            State   state;
            Type    type;

            Extra(): type(Type::EMPTY) {};
            Extra(Type _type, State _state): state(_state), type(_type) {};

            void reset() noexcept { type = Type::EMPTY; }

            [[nodiscard]] bool isPassable() const { return type != Type::BARRIER; }

            void hit(uint8_t damage) noexcept {
                if(type == Type::EMPTY) return;

                switch(state) {
                    case State::Hidden:
                        state = State::Unknown;
                        break;
                    case State::Unknown:
                        state = State::Visible;
                        break;
                    default:
                        break;
                }
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
                            default:
                                tile_index_offset = 60;
                        }
                        break;
                    case State::Taken:
                    default:
                        tile_index_offset = 64;
                }

                return tile_index_offset + (x | (y << 1));
            }
        };

        GameWorld(const SoundEngine& soundEngine);

        void clear() { 
            for(uint16_t i = 0; i < kMapTilesCount; ++i) { mTiles[i].reset(); } 
            for(uint16_t i = 0; i < kMapExtrasCount; ++i) { mExtraTiles[i].reset(); } 
        }

        void addLayer(const std::array<std::array<uint32_t, 64>, 256>& indices, Tile::Type layer_type, Tile::Flags flags) {
            static_assert(kMapWidth == 64);
            static_assert(kMapHeight == 256);

            for(uint16_t x = 0; x < kMapWidth; ++x) {
                for(uint16_t y = 0; y < kMapHeight; ++y) {
                    const uint32_t tile_index = indices[y][x];
                    if(tile_index == 0) continue;

                    mTiles[x + y * kMapWidth] = {tile_index - 1 /* Tiled editor indices are 1-based */, layer_type, flags};
                }
            }
        }

        template <typename T, std::size_t N>
        void addExtras(const std::array<T, N>& arr) {
            static_assert(kExtrasMapWidth == 32);
            static_assert(kExtrasMapHeight == 128);

            for(const auto& entry: arr) {
                uint16_t xx = entry.x >> 4;
                uint16_t yy = entry.y >> 4;

                auto& extra =  mExtraTiles[xx + yy * kExtrasMapWidth];
                extra.state = Extra::State::Visible; // for test !!!

                if(entry.type == "500") {
                    extra.type = Extra::Type::POINTS500;
                } else if(entry.type == "killall") {
                    extra.type = Extra::Type::KILLALLSCR;
                } else if(entry.type == "freeze") {
                    extra.type = Extra::Type::FREEZE10;
                } else if(entry.type == "life") {
                    extra.type = Extra::Type::EXTRALIFE;
                } else if(entry.type == "exit") {
                    extra.type = Extra::Type::EXIT;
                } else if(entry.type == "barrier") {
                    extra.type = Extra::Type::BARRIER;
                }
            }
        }

        // World map height in 8x8 tiles
        [[nodiscard]] uint16_t getMapHeight() const noexcept { return kMapHeight; }

        [[nodiscard]] const std::array<Tile, kMapTilesCount>& getTiles() const noexcept { return mTiles; }

        [[nodiscard]] const Tile& getBackgroundTile(uint32_t tile_index) const {
            assert(tile_index < mTiles.size()); 
            return mTiles[tile_index]; 
        }

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
        bool testMapCollitionAt(const GameObject& obj, float offset_x = 0.0f, float offset_y = 0.0f) const {
            const AABB& aabb = obj.getObstacleAABB();
            float x = obj.getPosX() + offset_x + aabb.x;
            float y = obj.getPosY() + offset_y + aabb.y;

            static const float sTileSizeF = static_cast<float>(kTileSize);

            uint16_t minTileX = std::max((int16_t)0, static_cast<int16_t>(std::floor(x / sTileSizeF)));
            uint16_t maxTileX = std::min(static_cast<int16_t>(std::floor((x + aabb.w - 0.001f) / sTileSizeF)), static_cast<int16_t>(kMapWidth - 1));
            uint16_t minTileY = std::max((int16_t)0, static_cast<int16_t>(std::floor(y / sTileSizeF)));
            uint16_t maxTileY = std::min(static_cast<int16_t>(std::floor((y + aabb.h - 0.001f) / sTileSizeF)), static_cast<int16_t>(kMapHeight - 1));

            for (uint16_t tx = minTileX; tx <= maxTileX; ++tx) {
                for (uint16_t ty = minTileY; ty <= maxTileY; ++ty) {
                    if (!isTilePassable(tx, ty)) {
                        return true; // Collision found!
                    }
                }
            }
            return false;
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
                if (testMapCollitionAt(obj, offset_x, offset_y)) {
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
                if (testMapCollitionAt(obj, offset_x, offset_y)) {
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


        void movePlayer(float dx, float dy) {
            if(!mpPlayer) return;

            static const float sViewportTopGap = 32.0f;
            static const float sViewportMaxX = 511.0f;
            static const float sViewportMaxY = 272.0f; // 16 pix at the bottom are status line

            const AABB& aabb = mpPlayer->getObstacleAABB();
            
            moveObject(*mpPlayer, dx, dy);
            
            // camera bounds with 16px gap on top
            mpPlayer->setPos(
                std::min(sViewportMaxX - aabb.w, std::max(0.0f - aabb.x, mpPlayer->getPosX())), 
                std::min((float)mCamera.getPosY() + sViewportMaxY - aabb.h, std::max((float)mCamera.getPosY() + sViewportTopGap - aabb.y, mpPlayer->getPosY()))
            );

            // check if trapped. then kill player
            if(testMapCollitionAt(*mpPlayer)) {
                mpPlayer->takeDamage(Character::kMaxDamage, true /* force kill */);
                mCamera.setDirection(Camera::Direction::NONE);
            }
        }

        void firePlayer() {
            if(!mpPlayer) return;
            mpPlayer->fire();
        }

        void spawnObject(std::unique_ptr<GameObject> pNewObject) override final {
            if (!pNewObject) return;

            mPendingObjects.push_back(std::move(pNewObject));
        }

        void update(float dt);

        const Player* getPlayer() const { return mpPlayer; }

        [[nodiscard]] bool isPlayerDead() const {
            return mpPlayer == nullptr;
        }

        // Is player dying in sinle player mode or last standing player is dying in multiplayer mode
        [[nodiscard]] bool isLastOrOnlyPlayerDying() const {
            return mpPlayer && mpPlayer->isDying();
        }

        Camera& getCamera() { return mCamera; }
        void setCameraHeight(uint16_t height) { mCamera.setHeight(height); }

        // Offset to camera visible tiles
        uint32_t getCurrentCameraTilesOffset() const {
            static constexpr float sTileHeightF = static_cast<float>(kTileSize);
            return static_cast<uint32_t>(std::ceil(static_cast<float>(mCamera.getPosY()) / sTileHeightF)) * kMapWidth;
        }

    private:
        SoundPlayer mSoundPlayer;
        Camera      mCamera;

        std::array<Extra, kMapExtrasCount> mExtraTiles; // Tiles that holds extra power ups
        std::array<Tile, kMapTilesCount> mTiles;

        std::vector<std::unique_ptr<GameObject>> mGameObjects; 
        std::vector<std::unique_ptr<GameObject>> mPendingObjects; // Deferred spawn list

        // A non-owning, weak shortcut pointer providing immediate access to the player instance
        Player* mpPlayer = nullptr; 
};

}  // namespace KnightGame

#endif  // __RETRO_CORE_TEST_GAME_WORLD_H

