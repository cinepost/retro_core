#ifndef __RETRO_CORE_LAUNCHER_INPUT_REMAPPING_H
#define __RETRO_CORE_LAUNCHER_INPUT_REMAPPING_H

#include <unordered_map>
#include <string>
#include <fstream>

#include "ini/ini.h"

namespace RetroLauncher {

enum class InputType {
    KEYBOARD,
    JOYSTICK_BUTTON,
    JOYSTICK_AXIS

    static std::string to_string(InputType input) {
        switch (input) {
            case InputType::KEYBOARD:
                return "Keyboard";
            case InputType::JOYSTICK_BUTTON:
                return "Joystick Button";
            case InputType::JOYSTICK_AXIS:
                return "Joystick Axis";
            default:
                assert(false);
                return "";
        }
    }
};

struct Binding {
    InputType type;
    int code;       // Keyboard scancode or joystick button index
    int axis_dir;   // Only used for AXIS: -1 for negative, +1 for positive
};

using PlayerMapping = std::unordered_map<unsigned, Binding>;
using PlayerMappings = std::unordered_map<unsigned, PlayerMapping>;

void saveCoreInputMappingsProfile(const std::string& core_path, const PlayerMappings& mappings) {
    mINI::INIStructure ini;
    std::string filename = getCoreConfigPath(core_path);

    // Ensure the input mapping config directory exists
    std::filesystem::create_directories("config");

    // Loop through all mapped ports (players)
    for (const auto& [port, player_map] : mappings) {
        std::string section = "Player_" + std::to_string(port);

        for (const auto& [retropad_id, bind] : player_map) {
            std::string key_prefix = "btn_" + std::to_string(retropad_id);

            // Save Type (0 = Keyboard, 1 = JoyButton, 2 = JoyAxis)
            ini[section][key_prefix + "_type"] = std::to_string(static_cast<int>(bind.type));
            ini[section][key_prefix + "_code"] = std::to_string(bind.code);
            ini[section][key_prefix + "_axis_dir"] = std::to_string(bind.axis_dir);
        }
    }

    // Write structure to file
    mINI::INIFile file(filename);
    file.generate(ini);
}

void loadCoreInputMappingsProfile(const std::string& core_path, PlayerMappings& mappings) {
    mappings.clear(); // Wipe active mappings

    std::string filename = getCoreConfigPath(core_path);
    
    // Fallback to a global profile if core-specific one doesn't exist yet
    if (!std::filesystem::exists(filename)) {
        filename = "config/global_fallback.ini";
        if (!std::filesystem::exists(filename)) {
            std::cerr << "No global_" << filename << " input remapping file exists!" << std::endl;
            return; // No configs found
        }
    }

    mINI::INIFile file(filename);
    mINI::INIStructure ini;
    file.read(ini);

    // Iterate through sections found in the INI file
    for (const auto& it : ini) {
        std::string section_name = it.first; // e.g., "Player_0"
        
        // Parse the port number out of the section header string
        if (section_name.find("Player_") != 0) continue;
        unsigned port = std::stoi(section_name.substr(7));

        // Gather all button settings inside this player's section
        for (const auto& key_it : it.second) {
            std::string key = key_it.first; // e.g., "btn_0_type"
            
            // We parse chunks using the "_type" string to discover the RetroPad IDs
            if (key.size() > 5 && key.substr(key.size() - 5) == "_type") {
                std::string id_str = key.substr(4, key.size() - 9);
                unsigned retropad_id = std::stoi(id_str);

                std::string key_prefix = "btn_" + id_str;

                Binding bind;
                bind.type = static_cast<InputType>(std::stoi(ini[section_name][key_prefix + "_type"]));
                bind.code = std::stoi(ini[section_name][key_prefix + "_code"]);
                bind.axis_dir = std::stoi(ini[section_name][key_prefix + "_axis_dir"]);

                mappings[port][retropad_id] = bind;
            }
        }
    }
}


}  // namespace RetroLauncher

#endif  // __RETRO_CORE_LAUNCHER_INPUT_REMAPPING_H