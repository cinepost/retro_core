#include "framework/sprites.h"

#include <limits>


namespace RetroCore {

template <PixelFormat FMT>
SpriteDataContainerBase::IndexType SpriteDataContainer<FMT>::appendSpriteData(uint32_t width, uint32_t height, const uint8_t* pSrc) {
	// Each source sprite data row should be byte aligned
	assert(bitsPerPixel(FMT) > 0);
	uint32_t raw_stride = bitsToBytesCount(width * bitsPerPixel(FMT));

	// Align individual rows to 64 bytes to optimize scanline thread reads and SIMD
	uint32_t aligned_stride = (raw_stride + 63) & ~63; 

	// Align start of entire sprite block to 64-byte boundary
	size_t current_size = mBulkSpriteData.size();
	size_t sprite_padding = ((current_size + 63) & ~63) - current_size;
	if (sprite_padding > 0) {
		mBulkSpriteData.insert(mBulkSpriteData.end(), sprite_padding, 0);
	}

	size_t start_offset = mBulkSpriteData.size();
	mHeaders.push_back({start_offset, width, height, aligned_stride});

	// Copy data row by row, adding padding at the end of each row
	mBulkSpriteData.resize(start_offset + (aligned_stride * height));
	for (uint32_t y = 0; y < height; ++y) {
		uint8_t* dest_row = &mBulkSpriteData[start_offset + (y * aligned_stride)];
		const uint8_t* src_row = &pSrc[y * raw_stride];
		std::memcpy(dest_row, src_row, raw_stride);
	}

	assert(mHeaders.size() <= std::numeric_limits<IndexType>::max());
	return static_cast<IndexType>(mHeaders.size() - 1);
}

}  // namespace RetroCore