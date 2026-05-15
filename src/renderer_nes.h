#ifndef __RETRO_CORE_RENDERER_NES_H
#define __RETRO_CORE_RENDERER_NES_H

#include "palette.h"
#include "renderer.h"

#include <cstdint>
#include <vector>
#include <cassert>
#include <vector>
#include <array>
#include <iostream>
#include <fstream>


namespace RetroCore {

class VDP_NES: public Renderer<VDP_Profile::NES> {
	public:
		VDP_NES();

	protected:
		virtual bool renderImpl(uint8_t* pFrameData, uint32_t stride_bytes) override;
};

}  // namespace RetroCore

#endif  // __RETRO_CORE_RENDERER_NES_H