#ifndef __RETRO_CORE_TYPES_H
#define __RETRO_CORE_TYPES_H

#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>


namespace RetroCore {

struct Coord {
	uint32_t x;
	uint32_t y;

	Coord(): x(0u), y(0u) {}
	Coord(uint32_t _x, uint32_t _y): x(_x), y(_y) {}
};

struct CoordRel {
	int x;
	int y;

	CoordRel(): x(0), y(0) {}
	CoordRel(int _x, int _y): x(_x), y(_y) {}
};

struct SpriteBase {
	uint16_t pos_x;
	uint16_t pos_y;
	uint16_t data_index;
};

template <uint8_t BPP>
struct SpriteData {
	uint16_t width;
	uint16_t height;
	static constexpr uint8_t bpp = BPP; // 2:RGB16, 3:RGB, 4:RGBA

	SpriteData(uint16_t w, uint16_t h);
	SpriteData(uint16_t w, uint16_t h, const uint8_t* pData);

	const uint8_t* data() const { return sprite_data.data(); }

	std::vector<uint8_t> sprite_data;
};

template <uint8_t BPP>
class SpriteDataContainer {
	public:
		SpriteDataContainer();

		const SpriteData<BPP>* getSpriteData(size_t index) {
			assert(index < mSpriteData.size());
			return mSpriteData[index];
		}

		size_t appendSpriteData(uint16_t w, uint16_t h, const uint8_t* pData) {
			mSpriteData.emplace_back({w, h, pData});
			return mSpriteData.size() - 1;
		}

	private:
		static constexpr uint8_t mBPP = BPP;

		std::vector<SpriteData<BPP>> mSpriteData;
};

}  // namespace RetroCore

#endif  // __RETRO_CORE_TYPES_H