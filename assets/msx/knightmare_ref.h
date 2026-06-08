#ifndef __RETRO_CORE_ASSETS_MSX_KNIGHTMARE_REF_H
#define __RETRO_CORE_ASSETS_MSX_KNIGHTMARE_REF_H

#include <cstdint>

namespace RetroCore {
namespace StaticData {
namespace MSX {

struct GimpTestImage {
  unsigned int 	width;
  unsigned int 	height;
  unsigned int 	bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
  uint8_t	      pixel_data[256 * 192 * 4 + 1];
};

extern const GimpTestImage gTestImage;

}  // namespace MSX
}  // namespace StaticData
}  // namespace RetroCore

#endif  // __RETRO_CORE_ASSETS_MSX_KNIGHTMARE_REF_H