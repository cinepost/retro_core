#ifndef __RETRO_CORE_FRAMEWORK_RENDERER_H
#define __RETRO_CORE_FRAMEWORK_RENDERER_H

#include "framework/vdp_utils.h"
#include "framework/types.h"
#include "framework/static_data.h"
#include "framework/ppu/ppu.h"

#include <cstdint>
#include <vector>
#include <cassert>
#include <limits>

namespace RetroCore {

class RendererBase {
	public:
		using Coord = RetroCore::Coord;
		using CoordRel = RetroCore::CoordRel;
		using CursorType = DebugData::CursorType;

		enum class BlitMode { 
			REPLACE,
			XOR,
			OVER, 
		};

		static constexpr uint8_t sPixelStride = 4; // 4 bytes per pixel

		virtual ~RendererBase() = default;

};

template <FramebufferDims FBDIMS, typename... PPUS>
//requires (std::is_base_of_v<PPU::PPU_BASE, PPUS> && ...) 
class Renderer: public RendererBase {
	public:
		static constexpr FramebufferDims sFramebufferSize = FBDIMS;
		static constexpr bool kDefaultShoeDebugInfoState = true;
		static constexpr uint16_t kMaxFramebufferAxisSize = std::numeric_limits<uint16_t>::max() / 2;
		static constexpr RendererBase::CursorType kDefaultDebugCursorType = RendererBase::CursorType::HAND;

	public:
		explicit Renderer(PPUS&... ppus):
		 	 mIsInitialized(false)
		 	,mIsFramebufferClear(false)
			,mIsExternalFramebufferClear(false)
			,mShowDebugInfo(kDefaultShoeDebugInfoState)
			,mDebugCursorType(kDefaultDebugCursorType)
			,mPPUs(ppus...)

		{
			static_assert(FBDIMS.width > 0);
			static_assert(FBDIMS.height > 0);
		}

		bool init();
		bool deinit();
		void reset();

		const uint8_t* render();
		const uint8_t* render(uint8_t* pFrameData, uint32_t stride_bytes);

		void  clearFramebuffer();
		void  clearFramebuffer(uint8_t* pFrameData, uint32_t stride_bytes);

		bool  isInitialized() const { return mIsInitialized; }

		static uint32_t getFramebufferStride() { 
			static constexpr uint32_t sFramebufferStride = FBDIMS.width * sPixelStride;
			return sFramebufferStride; 
		}
		static uint16_t getFramebufferWidth() { return FBDIMS.width; }
		static uint16_t getFramebufferHeight() { return FBDIMS.height; }

		void setShowDebugInfoState(bool state) { mShowDebugInfo = state; }
		bool getShowDebugInfoState() const { return mShowDebugInfo; }
		void setDebugBackgroundPos(uint32_t x, uint32_t y);
		void setDebugCursorPos(uint32_t x, uint32_t y);
		void moveDebugCursor(int16_t x_delta, int16_t y_delta);
		void setDebugCursorType(CursorType t) { mDebugCursorType = t; }

		const Coord& getDebugBackgroundPos() const { return mDebugBackgroundPos; }
		const Coord& getDebugCursorPos() const { return mDebugCursorPos; }

	private:
		bool _render(uint8_t* pFrameData, uint32_t stride_bytes, bool use_internal_buffer);
		void drawDebugBackground(uint8_t* pFrameData, uint32_t stride_bytes, uint32_t square_size = 16, uint32_t color1 = 0x00007F00, uint32_t color2 = 0x007F0000);
		void drawDebugCursor(uint8_t* pFrameData, uint32_t stride_bytes);

		template <BlitMode M>
		void blit(uint8_t* pFrameData, uint32_t stride_bytes, const uint8_t* pSrcData, uint16_t src_width, uint16_t src_height, int16_t dst_pos_x, int16_t dst_pos_y);
		
		void invertPixel(uint8_t* pFrameData, uint32_t stride_bytes, uint16_t x, uint16_t y);

	private:
		bool		mIsInitialized;
		bool        mIsFramebufferClear;
		bool        mIsExternalFramebufferClear;

		uint32_t    mFramebufferStride;
		size_t      mFramebufferDataSize;

		bool        mShowDebugInfo;
		CursorType  mDebugCursorType;
		Coord       mDebugBackgroundPos;
		Coord       mDebugCursorPos;

		alignas(64) std::array<uint8_t, FBDIMS.width * FBDIMS.height * RendererBase::sPixelStride> mFramebuffer;

	private:
		std::tuple<PPUS&...> mPPUs;
};

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_RENDERER_H