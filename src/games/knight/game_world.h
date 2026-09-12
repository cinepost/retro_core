#ifndef __RETRO_CORE_TEST_GAME_WORLD_H
#define __RETRO_CORE_TEST_GAME_WORLD_H

namespace KnightGame {

struct GameWorld {
    static const uint16_t kMapWidth = 64;
    static const uint16_t kMapHeight = 256;

    static const uint16_t kExtrasMapWidth = 32;
    static const uint16_t kExtrasMapHeight = 128;

    static constexpr uint16_t kMapTilesCount = kMapWidth * kMapHeight;
    static constexpr uint16_t kMapExtrasCount = kExtrasMapWidth * kExtrasMapHeight;

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

        inline void reset() { tile_index = 0; type = Type::None; flags = Flags::None; }
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

        inline void reset() { type = Type::EMPTY; }

        inline void hit(uint8_t damage) {
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

        inline uint16_t getTile(uint8_t x, uint8_t y) const {
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
            }
        }
    }

    inline const std::array<Tile, kMapTilesCount>& getTiles() const { return mTiles; }

    inline const Tile& getBackgroundTile(uint32_t tile_index) const { 
        return mTiles[tile_index]; 
    }

    inline const uint16_t getTileIndex(uint32_t tile_index) const { 
        uint16_t tile_y = tile_index >> 6;
        uint16_t tile_x = tile_index % kMapWidth; 

        uint16_t extra_tile_y = tile_y >> 1;
        uint16_t extra_tile_x = tile_x >> 1;
        uint16_t extra_tile = mExtraTiles[extra_tile_x + (extra_tile_y << 5)].getTile(tile_x % 2, tile_y % 2);

        //if(mExtraTiles[extra_tile_x + extra_tile_y << 5].type != Extra::Type::EMPTY) {
        //if(extra_tile_x % 2 || extra_tile_y % 2) {
        //    extra_tile = 12;
        //} else {
        //    extra_tile = 0;
        //}

        return extra_tile == 0 ? mTiles[tile_index].tile_index : extra_tile; 
    }

    std::array<Extra, kMapExtrasCount> mExtraTiles; // Tiles that holds extra power ups
    std::array<Tile, kMapTilesCount> mTiles;
};

}  // namespace KnightGame

#endif  // __RETRO_CORE_TEST_GAME_WORLD_H

