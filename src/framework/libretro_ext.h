#ifndef __RETRO_CORE_FRAMEWORK_LIBRETRO_EXT_H
#define __RETRO_CORE_FRAMEWORK_LIBRETRO_EXT_H

#define RETRO_ENVIRONMENT_CUSTOM_GET_HUD_DATA  0x5001
#define RETRO_ENVIRONMENT_CUSTOM_SET_HUD_CMD   0x5002

struct DeveloperHudData {
    uint32_t active_entities;
    float    cpu_frame_time_ms;
    char     current_map_name[64];
    uintptr_t memory_watch_ptr;
};

#endif  // __RETRO_CORE_FRAMEWORK_LIBRETRO_EXT_H