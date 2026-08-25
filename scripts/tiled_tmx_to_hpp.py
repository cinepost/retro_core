import xml.etree.ElementTree as ET
import os
import sys
import base64
import zlib
import gzip
import struct

def load_external_tsx_types(tmx_dir, root_element):
    """
    Parses referenced .tsx files to link Tile GIDs to their assigned Class/Type.
    """
    gid_type_map = {}
    
    for tileset in root_element.findall('tileset'):
        first_gid = int(tileset.get('firstgid', '1'))
        source_tsx = tileset.get('source')
        
        if source_tsx:
            # Resolve path relative to the TMX file location
            tsx_path = os.path.normpath(os.path.join(tmx_dir, source_tsx))
            if not os.path.exists(tsx_path):
                print(f"Warning: Tileset file not found at '{tsx_path}'")
                continue
                
            tsx_tree = ET.parse(tsx_path)
            tsx_root = tsx_tree.getroot()
            
            # Extract types defined on individual tiles inside the TSX
            for tile in tsx_root.findall('tile'):
                tile_id = int(tile.get('id', '0'))
                # Handle old 'type' and new 'class' attribute conventions
                tile_type = tile.get('class', tile.get('type', ''))
                
                if tile_type:
                    global_gid = first_gid + tile_id
                    gid_type_map[global_gid] = tile_type
    return gid_type_map

def tmx_to_cpp_header(tmx_path, header_path=None):
    if not os.path.exists(tmx_path):
        print(f"Error: File '{tmx_path}' not found.")
        return

    tmx_dir = os.path.dirname(tmx_path)

    # Parse TMX XML Structure
    tree = ET.parse(tmx_path)
    root = tree.getroot()

    # Pre-parse .tsx files to find default type definitions
    tsx_tile_types = load_external_tsx_types(tmx_dir, root)

    # Read Map Attributes
    map_width = root.get('width')
    map_height = root.get('height')
    
    # Generate clean variable name from filename
    base_name = os.path.splitext(os.path.basename(tmx_path))[0]
    var_name = "".join([c if c.isalnum() else "_" for c in base_name]).lower()

    if not header_path:
        header_path = f"{base_name}.hpp"

    with open(header_path, 'w') as f:
        # Include Guards
        f.write(f"#ifndef __{var_name.upper()}_MAP_H\n")
        f.write(f"#define __{var_name.upper()}_MAP_H\n\n")
        f.write("#include <array>\n")
        f.write("#include <cstdint>\n")
        f.write("#include <string_view>\n\n")

        # Dimensions constants
        f.write(f"const uint32_t {var_name.upper()}_WIDTH = {map_width};\n")
        f.write(f"const uint32_t {var_name.upper()}_HEIGHT = {map_height};\n\n")

        # --- Object Structure Definition ---
        f.write("struct TiledObject {\n")
        f.write("    uint32_t id;\n")
        f.write("    std::string_view name;\n")
        f.write("    std::string_view type;\n")
        f.write("    int x;\n")
        f.write("    int y;\n")
        f.write("    int width;\n")
        f.write("    int height;\n")
        f.write("};\n\n")

        # Iterate through all Tile Layers
        for layer in root.findall('layer'):
            layer_name = layer.get('name')
            layer_var_name = "".join([c if c.isalnum() else "_" for c in layer_name]).lower()
            data_element = layer.find('data')
            encoding = data_element.get('encoding')
            compression = data_element.get('compression')
            gids = []

            # Format 1: CSV Export
            if encoding == 'csv':
                raw_text = data_element.text.strip()
                gids = [x.strip() for x in raw_text.split(',') if x.strip()]
            
            # Format 2: Uncompressed XML export 
            elif encoding is None:
                for tile in data_element.findall('tile'):
                    gids.append(tile.get('gid', '0'))

            # Format 3: Base64 (Compressed or Uncompressed)
            elif encoding == 'base64':
                # Decode the base64 string back into raw bytes
                encoded_data = data_element.text.strip()
                compressed_bytes = base64.b64decode(encoded_data)
                
                # Uncompress if needed
                if compression == 'zlib':
                    raw_bytes = zlib.decompress(compressed_bytes)
                elif compression == 'gzip':
                    raw_bytes = gzip.decompress(compressed_bytes)
                elif compression is None:
                    raw_bytes = compressed_bytes
                else:
                    print(f"Skipping layer '{layer_name}': Compression '{compression}' not supported.")
                    continue
                
                # Tiled stores GIDs as 32-bit (4 byte) integers
                # Reconstruct integer array from the byte data
                for i in range(0, len(raw_bytes), 4):
                    gid = struct.unpack('<I', raw_bytes[i:i+4])[0]
                    gids.append(str(gid))
            else:
                print(f"Skipping layer '{layer_name}': Encoding '{encoding}' is not supported. Use CSV or XML.")
                continue

            # Write Multi-dimensional array 
            f.write(f"// Tile Layer: {layer_name}\n")
            f.write(f"inline constexpr std::array<std::array<uint32_t, {map_width}>, {map_height}> {var_name}_{layer_var_name}_map = {{{{\n")
            
            # Group into rows for visual readability
            w = int(map_width)
            for i in range(0, len(gids), w):
                row = gids[i:i+w]
                row_str = ", ".join(row)
                f.write(f"    {{ {row_str} }},\n")
                
            f.write("}};\n\n")

        # --- 2. PARSE OBJECT LAYERS ---
        for obj_group in root.findall('objectgroup'):
            group_name = obj_group.get('name')
            group_var_name = "".join([c if c.isalnum() else "_" for c in group_name]).lower()
            
            objects = obj_group.findall('object')
            obj_count = len(objects)

            f.write(f"// Object Layer: {group_name}\n")
            f.write(f"const size_t {var_name.upper()}_{group_var_name.upper()}_COUNT = {obj_count};\n")
            f.write(f"inline constexpr std::array<TiledObject, {obj_count}> {var_name}_{group_var_name} = {{\n    {{\n")

            for obj in objects:
                obj_id = obj.get('id', '0')
                obj_name = obj.get('name', '')
                obj_type = obj.get('class', obj.get('type', ''))
                obj_gid = obj.get('gid') # Check if it points to a tile graphic

                # Fallback 1: Resolve blank Type/Class via .tsx metadata lookups
                if not obj_type and obj_gid:
                    clean_gid = int(obj_gid) & 0x0FFFFFFF # Mask away Tiled's flip flags
                    obj_type = tsx_tile_types.get(clean_gid, '')

                # Fallback 2: General string fallbacks if still completely blank
                if not obj_name:
                    obj_name = f"object_{obj_id}"
                if not obj_type:
                    obj_type = "generic"

                # Spatial vectors default to zero if absent (e.g. single points)
                x = obj.get('x', '0')
                y = obj.get('y', '0')
                w = obj.get('width', '0')
                h = obj.get('height', '0')

                f.write(f'        {{ {obj_id}, "{obj_name}", "{obj_type}", {x}, {y}, {w}, {h} }},\n')

            f.write("    }\n};\n\n")

        f.write(f"#endif // __{var_name.upper()}_MAP_H\n")
    print(f"Successfully generated: {header_path}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python tiled_tmx_to_hpp.py <path_to_map.tmx>")
    else:
        out = sys.argv[2] if len(sys.argv) > 2 else None
        tmx_to_cpp_header(sys.argv[1], out)
