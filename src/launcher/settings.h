#ifndef __RETRO_CORE_LAUNCHER_SETTINGS_H
#define __RETRO_CORE_LAUNCHER_SETTINGS_H

#include <iostream>
#include <fstream>
#include <string>

#include "ini/ini.h"

struct RuntimeSettings {
    std::string         last_core_path = "";
    std::string         last_rom_path = "";
    float               audio_volume = 1.0f;
    float               screen_scale = 1.0f;
    bool                use_shaders = true;
    bool                aspect_ratio_lock = true;
    bool                fullscreen = true;
    bool                imgui_hud_show = false;
    bool                show_fps = false;
};

static std::string boolToString(bool b) {
    return b ? "true" : "false";
}

static bool stringToBool(const std::string& str, bool defaultValue) {
    if (str.empty()) return defaultValue;
    return (str == "true" || str == "1" || str == "yes");
}

void saveINI(const RuntimeSettings& settings) {
    mINI::INIStructure ini;
    
    // Populate structure
    ini["Frontend"]["LastCore"] = settings.last_core_path;
    ini["Frontend"]["LastRom"] = settings.last_rom_path;
    ini["Frontend"]["ImGuiHUD"] = boolToString(settings.imgui_hud_show);
    ini["Frontend"]["ShowFPS"] = boolToString(settings.show_fps);
    ini["Video"]["Scale"] = std::to_string(settings.screen_scale);
    ini["Video"]["AspectLock"] = boolToString(settings.aspect_ratio_lock);
    ini["Video"]["Fullscreen"] = boolToString(settings.fullscreen);
    ini["Video"]["UseShaders"] = boolToString(settings.use_shaders);
    ini["Audio"]["Volume"] = std::to_string(settings.audio_volume);

    mINI::INIFile file("config.ini");
    file.generate(ini);
}

void loadINI(RuntimeSettings& settings) {
    mINI::INIFile file("config.ini");
    mINI::INIStructure ini;
    
    if (file.read(ini)) {
        settings.last_core_path = ini["Frontend"]["LastCore"];
        settings.last_rom_path = ini["Frontend"]["LastRom"];
        if (ini["Frontend"].has("ImGuiHUD")) {
            settings.imgui_hud_show = stringToBool(ini["Frontend"]["ImGuiHUD"], false);
        }
        if (ini["Frontend"].has("ShowFPS")) {
            settings.show_fps = stringToBool(ini["Frontend"]["ShowFPS"], false);
        }
        if (ini["Video"].has("Scale")) {
            settings.screen_scale = std::stoi(ini["Video"]["Scale"]);
        }
        if (ini["Video"].has("AspectLock")) {
            settings.aspect_ratio_lock =settings.aspect_ratio_lock = stringToBool(ini["Video"]["AspectRatioLock"], true);
        }
        if (ini["Video"].has("Fullscreen")) {
            settings.fullscreen = stringToBool(ini["Video"]["Fullscreen"], true);
        }
        if (ini["Video"].has("UseShaders")) {
            settings.use_shaders = stringToBool(ini["Video"]["UseShaders"], true);
        }
        if (ini["Audio"].has("Volume")) {
            settings.audio_volume = std::stof(ini["Audio"]["Volume"]);
        }
    }
}

#endif  // __RETRO_CORE_LAUNCHER_SETTINGS_H