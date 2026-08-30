#include "game.h"
#include "framework/ppu/ppu_msx_utils.h"

#include "level_01.tmx.hpp"
#include "level_01_tiles.png.hpp"

#include <random> 

using namespace KnightGame;
using namespace RetroCore::PPU;

// Water
static auto water_pattern_0 = MSX_MAKE_PATTERN_8D_8C(48,A2,00,20,01,00,00,80,54,54,54,54,54,54,54,54);
static auto water_pattern_1 = MSX_MAKE_PATTERN_8D_8C(10,44,01,08,00,00,08,00,54,54,54,54,54,54,54,54);
static auto water_pattern_2 = MSX_MAKE_PATTERN_8D_8C(00,00,00,08,82,20,1C,FF,54,54,54,54,54,54,C4,C4); // with grass line
static auto water_pattern_3 = MSX_MAKE_PATTERN_8D_8C(00,00,00,10,41,04,38,FF,54,54,54,54,54,54,C4,C4); // with grass line

// Grass
static auto grass_pattern_0 = MSX_MAKE_PATTERN_8D_8C(05,20,10,82,00,20,04,00,AC,AC,AC,AC,AC,AC,AC,AC);
static auto grass_pattern_1 = MSX_MAKE_PATTERN_8D_8C(01,80,14,2A,18,1E,40,12,AC,AC,AC,3C,2C,1C,AC,AC);
static auto grass_pattern_2 = MSX_MAKE_PATTERN_8D_8C(82,10,25,2A,1E,80,10,41,AC,AC,AC,3C,1C,AC,AC,AC);
static auto grass_pattern_3 = MSX_MAKE_PATTERN_8D_8C(50,06,06,40,09,60,60,60,AC,EC,1C,AC,AC,EC,EC,1C);
static auto grass_pattern_4 = MSX_MAKE_PATTERN_8D_8C(00,00,00,00,00,20,04,00,E1,E1,E1,E1,AC,AC,AC,AC); // shadow block _ 
static auto grass_pattern_5 = MSX_MAKE_PATTERN_8D_8C(1F,1F,3F,3F,FF,20,04,00,C1,C1,C1,C1,C1,AC,AC,AC); // shadow block /

// Grass mirrored
static auto grass_pattern_6 = MSX_MAKE_PATTERN_8D_8C(A0,04,08,41,00,04,20,00,AC,AC,AC,AC,AC,AC,AC,AC);
static auto grass_pattern_7 = MSX_MAKE_PATTERN_8D_8C(80,01,28,54,18,78,02,48,AC,AC,AC,3C,2C,1C,AC,AC);
static auto grass_pattern_8 = MSX_MAKE_PATTERN_8D_8C(41,08,A4,54,78,01,08,82,AC,AC,AC,3C,1C,AC,AC,AC);
static auto grass_pattern_9 = MSX_MAKE_PATTERN_8D_8C(0A,60,60,02,90,06,06,06,AC,EC,1C,AC,AC,EC,EC,1C);
static auto grass_pattern_10 = MSX_MAKE_PATTERN_8D_8C(00,00,00,00,00,04,20,00,E1,E1,E1,E1,AC,AC,AC,AC); // shadow block _
static auto grass_pattern_11 = MSX_MAKE_PATTERN_8D_8C(F8,F8,FC,FC,FF,04,20,00,C1,C1,C1,C1,C1,AC,AC,AC); // shadow block /

// Wooden planks
static auto plank_pattern_0 = MSX_MAKE_PATTERN_8D_8C(80,C5,00,00,F1,C0,FF,7F,1A,1A,1A,1A,1A,1A,1A,14);
static auto plank_pattern_1 = MSX_MAKE_PATTERN_8D_8C(00,80,00,00,D8,00,FF,FF,1A,1A,1A,1A,1A,1A,1A,1A);
static auto plank_pattern_2 = MSX_MAKE_PATTERN_8D_8C(00,01,00,00,0E,00,FF,FF,1A,1A,1A,1A,1A,1A,1A,1A);
static auto plank_pattern_3 = MSX_MAKE_PATTERN_8D_8C(07,6F,03,03,5F,07,FF,FE,1A,1A,1A,1A,1A,1A,1A,14);


static_assert(LevelBaseState::check_extras_layer(level_01_extras), "All level 1 extras positions must be divisible by 16 and size 16x16 !");


void Level1State::enter() {
    LevelBaseState::enter();

    // Prepare level map

    mWorld.addLayer(level_01_ground_map, GameWorld::Tile::Type::Ground, GameWorld::Tile::Flags::None);
    mWorld.addLayer(level_01_columns_map, GameWorld::Tile::Type::Wall, GameWorld::Tile::Flags::None);
    mWorld.addLayer(level_01_rivers_map, GameWorld::Tile::Type::Water, GameWorld::Tile::Flags::None);
    mWorld.addLayer(level_01_bridges_map, GameWorld::Tile::Type::Bridge, GameWorld::Tile::Flags::None);
    mWorld.addLayer(level_01_end_map, GameWorld::Tile::Type::Wall, GameWorld::Tile::Flags::None);

    mWorld.addExtras(level_01_extras);

/*
    RetroCore::Palette<16> img_palette;
    static const std::string mockup_filename = "/home/max/mnt/misc_hdd/dev/retro_core/games/virt_msx/KnightmareW/level_1_mockup_01.png";
    
    if(mPPU.loadIndexedImagePNG(mockup_filename, mPPU.getVramPageAddress(0), 512, 576, &img_palette)) {
        mPPU.setPalette(img_palette);
    }

    static const std::string test_status_line_filename = "/home/max/mnt/misc_hdd/dev/retro_core/games/virt_msx/KnightmareW/test_status_line.png";
    if(!mPPU.loadIndexedImagePNG(test_status_line_filename, mPPU.getVramPageAddress(2), 512, 16)) {
        std::cerr << "Error loading " << test_status_line_filename << std::endl; 
    }
*/

    // Load level tiles
    mPPU.pushTiles(level_1_tiles_tiles.data(), LEVEL_1_TILES_TILE_COUNT, 0 /* first tile offset */);

    // Test sprites
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<int16_t> distr_x(-7, 512);
    std::uniform_int_distribution<int16_t> distr_y(-7, 288);
    std::bernoulli_distribution d(0.5); // Bernoulli distribution (50% chance of true/false)

    uint16_t pattern = 0;
    for(uint16_t i = 0; i < 120; ++i) {
        mSprites.push_back({pattern, distr_x(gen), distr_y(gen), d(gen) ? 1 : -1, d(gen) ? 1 : -1});
        pattern += 4;
    }

    // MP3 audio test playback

    static const std::string level_bgm_mp3_filename = "/home/max/mnt/misc_hdd/dev/retro_core/games/virt_msx/KnightmareW/music/suno_level_01_bgm_01.mp3";
    const uint8_t* mp3File = mAssetManager.getFile(level_bgm_mp3_filename);

    if(mp3File) {
        size_t file_size = mAssetManager.getFileSize(level_bgm_mp3_filename);

        // 1. Initialize the MP3 asset locally within this state context
        mBgmMp3Stream = std::make_unique<GameEngine::MP3Stream>(mp3File, file_size, true);
    } else {
        std::cerr << "Error loading file " << level_bgm_mp3_filename << std::endl;
    }
}

void Level1State::exit() {
    mPPU.setBlankingBit(true);
}

void Level1State::handleInput(retro_input_state_t input_cb) {

}

void Level1State::update(double dt) {
    LevelBaseState::update(dt);
}

void Level1State::render() {

    uint16_t sprite_id = 0;
    uint8_t color = 0;
    for(const auto& sprite: mSprites) {
        MsxPPU_BASE::Sprite& hw_sprite = mPPU.getSpriteAttribute(sprite_id++);
        hw_sprite.x = sprite.pos_x;
        hw_sprite.y = sprite.pos_y;
        hw_sprite.index = sprite.pattern;
        hw_sprite.attribs.color = color++;

        if(color == 16) color = 0;
    }
}

void Level1State::renderAudio(int16_t* pSamplesData, size_t samples_per_frame)  {
    if(mBgmMp3Stream) mBgmMp3Stream->renderPCM(pSamplesData, samples_per_frame);
}