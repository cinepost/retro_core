import sys
import os

if len(sys.argv) < 4:
    print("Usage: python chr_to_cpp.py <input.chr> <output.cpp> <array_name>")
    sys.exit(1)

input_path, output_path, array_name = sys.argv[1:4]

with open(input_path, "rb") as f:
    data = f.read()

# Generate the C++ code
cpp_content = f"""#include <cstdint>
#include <array>

namespace RetroCore {{
namespace StaticData {{
namespace NES {{
    // CHR Data: {os.path.basename(input_path)} ({len(data)} bytes)
    extern const std::array<uint8_t, {len(data)}> {array_name} = {{
"""

# Format bytes into clean hex blocks (16 bytes per line)
hex_lines = []
for i in range(0, len(data), 16):
    chunk = data[i:i+16]
    hex_line = "        " + ", ".join(f"0x{b:02X}" for b in chunk)
    if i + 16 < len(data):
        hex_line += ","
    hex_lines.append(hex_line)

cpp_content += "\n".join(hex_lines) + "\n    };\n}\n}\n}\n"

# Ensure the output directory tree exists before writing
output_dir = os.path.dirname(output_path)
if output_dir:
    os.makedirs(output_dir, exist_ok=True)

with open(output_path, "w") as f:
    f.write(cpp_content)
