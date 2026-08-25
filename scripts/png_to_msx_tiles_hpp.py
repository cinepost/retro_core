import sys
import os
from PIL import Image

def png_to_msx_tiles(png_path, hpp_path=None):
    if not os.path.exists(png_path):
        print(f"Error: File '{png_path}' not found.")
        return

    # Open image and force verify it uses an Indexed palette
    img = Image.open(png_path)
    if img.mode != 'P':
        print(f"Error: '{png_path}' is not an Indexed (Palette) PNG. Please convert it first.")
        return

    width, height = img.size
    if width % 8 != 0 or height % 8 != 0:
        print(f"Warning: Image dimensions ({width}x{height}) are not multiples of 8. Edge pixels will be skipped.")

    # Calculate total tiles (Grid ordered: left-to-right, top-to-bottom)
    tiles_x = width // 8
    tiles_y = height // 8
    total_tiles = tiles_x * tiles_y

    base_name = os.path.splitext(os.path.basename(png_path))[0]
    var_name = "".join([c if c.isalnum() else "_" for c in base_name]).lower()

    if not hpp_path:
        hpp_path = f"{base_name}_tiles.hpp"

    output_lines = []
    
    # Process each 8x8 block
    for ty in range(tiles_y):
        for tx in range(tiles_x):
            tile_bytes = [0] * 16  # 8 bytes pattern, 8 bytes color
            
            # Read 8 rows inside the tile
            for row in range(8):
                py = ty * 8 + row
                pattern_byte = 0
                
                # Sample the pixels and determine the two primary palette indexes
                row_pixels = []
                for col in range(8):
                    px = tx * 8 + col
                    # Safe boundaries check
                    if px < width and py < height:
                        row_pixels.append(img.getpixel((px, py)))
                    else:
                        row_pixels.append(0)

                # Find unique palette colors on this specific row
                unique_colors = list(set(row_pixels))
                
                # MSX constraints limit a row to 2 colors max
                bg_color = unique_colors[0]
                fg_color = unique_colors[1] if len(unique_colors) > 1 else bg_color

                # Build the pattern byte (1 bit per pixel)
                for col in range(8):
                    pixel_color = row_pixels[col]
                    if pixel_color == fg_color and fg_color != bg_color:
                        pattern_byte |= (1 << (7 - col)) # Bit set to 1 for Foreground

                # Combine colors into a single byte (FG=High Nibble, BG=Low Nibble)
                # Keep colors clamped within MSX 4-bit palette range (0-15)
                color_byte = ((fg_color & 0x0F) << 4) | (bg_color & 0x0F)

                tile_bytes[row] = pattern_byte        # Bytes 0-7: Bitmap Pattern
                tile_bytes[row + 8] = color_byte      # Bytes 8-15: Attribute Colors

            # Convert to hex strings
            hex_values = [f"0x{b:02X}" for b in tile_bytes]
            output_lines.append("    { " + ", ".join(hex_values) + " }")

    # Generate C++ HPP contents
    with open(hpp_path, 'w') as f:
        f.write(f"#ifndef __{var_name.upper()}_TILES_HPP\n")
        f.write(f"#define __{var_name.upper()}_TILES_HPP\n\n")
        f.write("#include <array>\n")
        f.write("#include <cstdint>\n\n")
        
        f.write(f"// Total Tiles Extracted: {total_tiles}\n")
        f.write(f"const size_t {var_name.upper()}_TILE_COUNT = {total_tiles};\n\n")
        
        f.write(f"inline const std::array<std::array<uint8_t, 16>, {total_tiles}> {var_name}_tiles = {{{{\n")
        f.write(",\n".join(output_lines))
        f.write("\n}};\n\n")
        
        f.write(f"#endif // __{var_name.upper()}_TILES_HPP\n")

    print(f"Generated {total_tiles} MSX tiles -> {hpp_path}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python png_to_msx.py <spritesheet.png> [output.hpp]")
    else:
        out = sys.argv[2] if len(sys.argv) > 2 else None
        png_to_msx_tiles(sys.argv[1], out)
