#ifndef __RETRO_CORE_FRAMEWORK_VDP_NES_H
#define __RETRO_CORE_FRAMEWORK_VDP_NES_H

#include "palette.h"
#include "framework/vdp_base.h"

#include <cstdint>
#include <vector>
#include <cassert>
#include <vector>
#include <array>
#include <iostream>
#include <fstream>


namespace RetroCore {

class VDP_NES: public Renderer<Platform::NES> {
	public:
		VDP_NES(uint16_t framebuffer_width, uint16_t framebuffer_height): Renderer<Platform::NES>(framebuffer_width, framebuffer_height) {

		}
		
	protected:
		bool initImpl() override;
		bool deinitImpl() override;
		bool renderImpl(void* pFrameData, uint32_t stride_bytes) override;
};

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_VDP_NES_H