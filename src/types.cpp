#include "types.h"


namespace RetroCore {

template <uint8_t BPP>
SpriteData<BPP>::SpriteData(uint16_t w, uint16_t h): width(w), height(h), sprite_data(w * h * BPP) {

}

template <uint8_t BPP>
SpriteData<BPP>::SpriteData(uint16_t w, uint16_t h, const uint8_t* pData): SpriteData(w, h) {
	assert(pData);
	assert(sprite_data.size() == w * h * BPP);
	std::memcpy(sprite_data.data(), pData, w * h * BPP);
}

template <uint8_t BPP>
SpriteDataContainer<BPP>::SpriteDataContainer() {
    // Implementation here
}

}  // namespace RetroCore