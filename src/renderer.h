#ifndef __RETRO_CORE_RENDERER_H
#define __RETRO_CORE_RENDERER_H

#include "debug_data.h"
#include "types.h"

#include <cstdint>
#include <vector>
#include <cassert>


namespace RetroCore {

class Renderer {
	public:
		using Coord = RetroCore::Coord;
		using CoordRel = RetroCore::CoordRel;
		using CursorType = DebugData::CursorType;

		enum class BlitMode { 
			REPLACE,
			XOR,
			OVER, 
		};

		Renderer();

		bool init(uint16_t framebuffer_width, uint16_t framebuffer_height);
		void deinit();
		void reset();

		const uint8_t* render();
		const uint8_t* render(uint8_t* pFrameData, uint32_t stride_bytes);

		void  clearFramebuffer();
		void  clearFramebuffer(uint8_t* pFrameData, uint32_t stride_bytes);

		bool  isInitialized() const { return mIsInitialized; }

		uint32_t getFramebufferStride() const { return mFramebufferStride; }
		uint16_t getFramebufferWidth() const { return mFramebufferWidth; }
		uint16_t getFramebufferHeight() const { return mFramebufferHeight; }

		void setShowDebugInfoState(bool state) { mShowDebugInfo = state; }
		bool getShowDebugInfoState() const { return mShowDebugInfo; }
		void setDebugBackgroundPos(uint32_t x, uint32_t y);
		void setDebugCursorPos(uint32_t x, uint32_t y);
		void moveDebugCursor(int16_t x_delta, int16_t y_delta);
		void setDebugCursorType(CursorType t) { mDebugCursorType = t; }

		const Coord& getDebugBackgroundPos() const { return mDebugBackgroundPos; }
		const Coord& getDebugCursorPos() const { return mDebugCursorPos; }

	protected:
		virtual bool renderImpl(uint8_t* pFrameData, uint32_t stride_bytes) = 0;

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

		uint16_t 	mFramebufferWidth;
		uint16_t 	mFramebufferHeight;
		uint8_t     mPixelStride;
		uint32_t    mFramebufferStride;
		size_t      mFramebufferDataSize;

		bool        mShowDebugInfo;
		CursorType  mDebugCursorType;
		Coord       mDebugBackgroundPos;
		Coord       mDebugCursorPos;

		std::vector<uint8_t> mFramebuffer;
};

}  // namespace RetroCore

#endif  // __RETRO_CORE_RENDERER_H