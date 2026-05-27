#ifndef __RETRO_CORE_FRAMEWORK_PPU_PPU_BASE_H
#define __RETRO_CORE_FRAMEWORK_PPU_PPU_BASE_H

#include "framework/vdp_utils.h"
#include "framework/types.h"
#include "framework/static_data.h"

#include <cstdint>
#include <vector>
#include <cassert>
#include <limits>
#include <variant>

namespace RetroCore {

namespace PPU {

template <typename ENTRY, uint16_t LINE_SIZE, uint16_t LINES>
class CRAM {
	public:
		using CRAM_Line = std::array<ENTRY, LINE_SIZE>;

		const CRAM_Line& getCRAMLine(uint8_t index) const {
			return (CRAM_Line&)getCRAMLine(index);
		}

		CRAM_Line& getCRAMLine(uint8_t index) {
			static constexpr uint8_t sMask = getIndexMask<uint8_t, uint16_t>(LINES);
			return mCRAMLines[index & sMask];
		}

	private:
		std::array<CRAM_Line, LINES> mCRAMLines;
};


class PPU_BASE {
	public:
		using DebugRegisterValue = std::variant<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t>; 
		using DebugRegisters = std::vector<std::pair<std::string, DebugRegisterValue>>;
		const DebugRegisters& getDebugRegisters() const { return mDebugRegisters; }

	protected:
		DebugRegisters mDebugRegisters;
};


template <Platform VDP>
class Abstract_PPU: public PPU_BASE {
	public:
		Abstract_PPU() = default;
		virtual ~Abstract_PPU() = default; 

	public:
		virtual void reset() {}
		virtual bool init() { return true; }
		virtual bool deinit() { return true; }
		virtual bool render(uint8_t* pFrameData, uint32_t stride_bytes) = 0; // Render to host (e.g libretro) framebuffer
		virtual void renderDebugScreen(uint8_t* pFrameData, uint32_t stride_bytes) {}


	protected:
		static constexpr PixelFormat 		mNativeFramebufferPixelFormat = getProfileNativeFramebufferPixelFormat(VDP);
		static constexpr uint8_t     		mNativePixelStride = bytesPerPixel(getProfileNativeFramebufferPixelFormat(VDP));
};

}  // namspace PPU

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_PPU_PPU_BASE_H