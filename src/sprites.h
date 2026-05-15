#ifndef __RETRO_CORE_SPRITES_H
#define __RETRO_CORE_SPRITES_H

#include "formats.h"

#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>


namespace RetroCore {

struct SpriteBase {
	uint16_t pos_x;
	uint16_t pos_y;
	uint16_t data_index;
};

struct SpriteHeader {
    uint32_t data_offset;
    uint32_t width;
    uint32_t height;
    uint32_t stride; // Actual byte width of a row in memory, including padding
};

struct SpriteDataContainerBase {
	using IndexType = uint32_t;
};

template <PixelFormat FMT>
class SpriteDataContainer: public SpriteDataContainerBase {
	public:
		SpriteDataContainer();

		IndexType appendSpriteData(uint32_t width, uint32_t height, const uint8_t* raw_pixels);

		// Thread-safe scanline pixel pointer lookup
		const uint8_t* getSpriteRowData(uint32_t index, uint32_t row) const {
			const SpriteHeader& h = mHeaders[index];
			return &mBulkSpriteData[h.data_offset + (row * h.stride)];
		}

	private:
		static constexpr uint8_t fmt = FMT;

		std::vector<SpriteHeader> mHeaders;
    	alignas(64) std::vector<uint8_t> mBulkSpriteData;
};

}  // namespace RetroCore

#endif  // __RETRO_CORE_SPRITES_H