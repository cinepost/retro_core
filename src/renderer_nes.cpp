#include "renderer_nes.h"

namespace RetroCore {

VDP_NES::VDP_NES():Renderer<VDP_Profile::NES>() {
	
}

bool VDP_NES::renderImpl(uint8_t* pFrameData, uint32_t stride_bytes) {

	return true;
}

}  // namespace RetroCore