import os
import sys
from PIL import Image

def png_to_msx_header(png_path, header_path, sprite_size = 16):
    # Open the image and ensure it's loaded as a 2-color indexed or grayscale image
    img = Image.open(png_path)
    img = img.convert('1')  # Force 1-bit pixels, black and white
    
    if sprite_size != 8 and sprite_size != 16:
        raise ValueError("Sprite size must be 8x8 or 16x16!")

    sprite_size = int(sprite_size)
    bytes_per_sprite = int(sprite_size * sprite_size / 8);

    width, height = img.size
    
    # Validate dimensions are multiples of 8 or 16
    if width % sprite_size != 0 or height % sprite_size != 0:
        raise ValueError(f"Image dimensions must be multiples of {sprite_size}x{sprite_size}!")
        
    cols = width // sprite_size
    rows = height // sprite_size
    total_sprites = cols * rows
    
    base_name = os.path.splitext(os.path.basename(png_path))[0].upper()
    
    with open(header_path, 'w') as f:
        # Write C++ header guards
        f.write(f"#ifndef __{base_name.upper()}_SPRITES_H\n")
        f.write(f"#define __{base_name.upper()}_SPRITES_H\n\n")
        f.write(f"#define {base_name.upper()}_SPRITE_COUNT {total_sprites}\n")
        f.write(f"#define {base_name.upper()}_BYTES_PER_SPRITE {bytes_per_sprite}\n\n")
        f.write(f"// MSX {total_sprites} {sprite_size}x{sprite_size} Sprites Data ({bytes_per_sprite} bytes per sprite)\n")
        f.write(f"const unsigned char {base_name}_DATA[{total_sprites}][{bytes_per_sprite}] = {{\n")
        
        # Grid loop to slice 16x16 blocks
        for r in range(rows):
            for c in range(cols):
                f.write("    {\n        ")
                
                subcol_count = sprite_size >> 3
                for subcol in range(subcol_count):

                    # Extract 16x16 sprite data
                    for y in range(sprite_size):
                        pix_byte = 0
                        
                        # Process the 8 pixels of the current row
                        for x in range(8):
                            pixel_x = (c * sprite_size + subcol * 8) + x
                            pixel_y = (r * sprite_size) + y
                            
                            # 0 is usually black/color, 255 is white. 
                            # We invert it so colored/black pixels = 1 (active sprite pixel)

                            pixel = img.getpixel((pixel_x, pixel_y))
                            bit = 0 if pixel == 0 else 1
                            
                            pix_byte |= (bit << (7 - x))
                         
                        f.write(f"0x{pix_byte:02X}")
                        
                        if y < (sprite_size - 1):
                            if (y + 1) % 8 == 0:
                                f.write(",\n        ")
                            else:
                                f.write(", ")

                    if subcol == 0 and subcol_count == 2:
                        f.write(",\n        ")
                                
                f.write("\n    }")
                if (r * cols + c) < (total_sprites - 1):
                    f.write(",\n")
                else:
                    f.write("\n")
                    
        f.write("};\n\n")
        f.write(f"#endif  // __{base_name.upper()}_TILES_HPP\n")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python png_to_msx_sprites_hpp.py <spritesheet.png> [output.hpp]")
    else:
        input_path = sys.argv[1]
        output_header = sys.argv[2] if len(sys.argv) > 2 else (input_path + ".hpp")
    
    try:
        png_to_msx_header(input_path, output_header)
        print(f"Successfully generated {output_header}!")
    except Exception as e:
        print(f"Error: {e}")
