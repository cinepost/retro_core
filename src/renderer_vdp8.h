#ifndef __RETRO_CORE_RENDERER_VDP8_H
#define __RETRO_CORE_RENDERER_VDP8_H

#include "renderer.h"

#include <cstdint>
#include <vector>
#include <cassert>
#include <vector>
#include <array>
#include <iostream>
#include <fstream>


namespace RetroCore {

class VDP8: public Renderer {
	public:
		VDP8();

		struct Palette {
			using Color = std::array<uint8_t, 3>;
			std::vector<Color> colors;

			Palette() { }
			Palette(const std::array<uint8_t, 192>& data): colors(64) { 
				for (size_t i = 0; i < 64; ++i) {
					size_t ii = i * 3;
					colors[i] = {data[ii++], data[ii++], data[ii]};
				}
			}

			const Color* data() const { return colors.data(); }
			uint32_t colorsCount() const { return colors.size(); }

			const Color& getNESColor(uint8_t index) {
        		assert(isNESPalette());
        		return colors[index & 0x3F];
    		}

			bool isNESPalette() const { return colorsCount() == 64; }
		};

		bool loadNESPalette(const std::string& filename);

	protected:
		virtual bool renderImpl(uint8_t* pFrameData, uint32_t stride_bytes) override;


	private:
		Palette mPalette;
};

}  // namespace RetroCore

#endif  // __RETRO_CORE_RENDERER_VDP8_H