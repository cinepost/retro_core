#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "libretro.h"

#include <iostream>
#include <map>
#include <vector>
#include <cmath>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>

#include <stdarg.h>
#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #define DLOPEN(path)          LoadLibraryA(path)
    #define DLSYM(handle, name)   GetProcAddress((HMODULE)handle, name)
    #define DLCLOSE(handle)       FreeLibrary((HMODULE)handle)
    #define DLERROR()             "Windows dynamic load error"
#else
    #include <dlfcn.h>
    #define DLOPEN(path)          dlopen(path, RTLD_LAZY)
    #define DLSYM(handle, name)   dlsym(handle, name)
    #define DLCLOSE(handle)       dlclose(handle)
    #define DLERROR()             dlerror()
#endif

#include "settings.h"
#include "crt.h"

RuntimeSettings g_settings;

// 1. The Frontend Logger function
void cb_log_printf(enum retro_log_level level, const char *fmt, ...) {
    const char *level_str = "DEBUG";
    switch (level) {
        case RETRO_LOG_DEBUG: level_str = "[CORE DEBUG] "; break;
        case RETRO_LOG_INFO:  level_str = "[CORE INFO] ";  break;
        case RETRO_LOG_WARN:  level_str = "[CORE WARN] ";  break;
        case RETRO_LOG_ERROR: level_str = "[CORE ERROR] "; break;
        default: level_str = "[CORE] ";break;
    }

    printf("[%s] ", level_str);
    
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

// 1. Libretro Pixel Format Constant Defs (Add if missing)

// Function pointer signatures
typedef void (*retro_init_t)(void);
typedef void (*retro_deinit_t)(void);
typedef unsigned (*retro_api_version_t)(void);
typedef void (*retro_get_system_info_t)(struct retro_system_info *info);
typedef void (*retro_get_system_av_info_t)(struct retro_system_av_info *info);
typedef void (*retro_set_environment_t)(bool (*environ_cb)(unsigned cmd, void *data));
typedef void (*retro_set_video_refresh_t)(void (*video_cb)(const void *data, unsigned width, unsigned height, size_t pitch));
typedef void (*retro_set_audio_sample_t)(void (*audio_cb)(int16_t left, int16_t right));
typedef void (*retro_set_audio_sample_batch_t)(size_t (*audio_batch_cb)(const int16_t *data, size_t frames));
typedef void (*retro_set_input_poll_t)(void (*input_poll_cb)(void));
typedef void (*retro_set_input_state_t)(int16_t (*input_state_cb)(unsigned port, unsigned device, unsigned index, unsigned id));
typedef bool (*retro_load_game_t)(const struct retro_game_info *game);
typedef void (*retro_log_printf_t)(enum retro_log_level level, const char *fmt, ...);
typedef void (*retro_unload_game_t)(void);
typedef void (*retro_run_t)(void);

// Adjustable Controller Deadzone (SDL axes report values between -32768 and 32767)
const int16_t ANALOG_DEADZONE = 4000;

// Global input map translating Libretro button IDs to SDL Scancodes

const std::map<SDL_Scancode, unsigned> g_raw_keyboard_map = {
    { SDL_SCANCODE_A,          97  }, // RETROK_a
    { SDL_SCANCODE_B,          98  }, // RETROK_b
    { SDL_SCANCODE_C,          99  }, // RETROK_c
    { SDL_SCANCODE_D,          100 }, // RETROK_d
    { SDL_SCANCODE_E,          101 }, // RETROK_e
    { SDL_SCANCODE_F,          102 }, // RETROK_f
    { SDL_SCANCODE_G,          103 }, // RETROK_g
    { SDL_SCANCODE_H,          104 }, // RETROK_h
    { SDL_SCANCODE_I,          105 }, // RETROK_i
    { SDL_SCANCODE_J,          106 }, // RETROK_j
    { SDL_SCANCODE_K,          107 }, // RETROK_k
    { SDL_SCANCODE_L,          108 }, // RETROK_l
    { SDL_SCANCODE_M,          109 }, // RETROK_m
    { SDL_SCANCODE_N,          110 }, // RETROK_n
    { SDL_SCANCODE_O,          111 }, // RETROK_o
    { SDL_SCANCODE_P,          112 }, // RETROK_p
    { SDL_SCANCODE_Q,          113 }, // RETROK_q
    { SDL_SCANCODE_R,          114 }, // RETROK_r
    { SDL_SCANCODE_S,          115 }, // RETROK_s
    { SDL_SCANCODE_T,          116 }, // RETROK_t
    { SDL_SCANCODE_U,          117 }, // RETROK_u
    { SDL_SCANCODE_V,          118 }, // RETROK_v
    { SDL_SCANCODE_W,          119 }, // RETROK_w
    { SDL_SCANCODE_X,          120 }, // RETROK_x
    { SDL_SCANCODE_Y,          121 }, // RETROK_y
    { SDL_SCANCODE_Z,          122 }, // RETROK_z
    { SDL_SCANCODE_1,          49  }, // RETROK_1
    { SDL_SCANCODE_2,          50  }, // RETROK_2
    { SDL_SCANCODE_3,          51  }, // RETROK_3
    { SDL_SCANCODE_4,          52  }, // RETROK_4
    { SDL_SCANCODE_5,          53  }, // RETROK_5
    { SDL_SCANCODE_6,          54  }, // RETROK_6
    { SDL_SCANCODE_7,          55  }, // RETROK_7
    { SDL_SCANCODE_8,          56  }, // RETROK_8
    { SDL_SCANCODE_9,          57  }, // RETROK_9
    { SDL_SCANCODE_0,          48  }, // RETROK_0
    { SDL_SCANCODE_RETURN,     13  }, // RETROK_RETURN
    { SDL_SCANCODE_ESCAPE,     27  }, // RETROK_ESCAPE
    { SDL_SCANCODE_BACKSPACE,  8   }, // RETROK_BACKSPACE
    { SDL_SCANCODE_TAB,        9   }, // RETROK_TAB
    { SDL_SCANCODE_SPACE,      32  }  // RETROK_SPACE
};

const std::map<unsigned, SDL_Scancode> g_key_map = {
    { RETRO_DEVICE_ID_JOYPAD_UP,     SDL_SCANCODE_UP     }, // D-Pad Up
    { RETRO_DEVICE_ID_JOYPAD_DOWN,   SDL_SCANCODE_DOWN   }, // D-Pad Down
    { RETRO_DEVICE_ID_JOYPAD_LEFT,   SDL_SCANCODE_LEFT   }, // D-Pad Left
    { RETRO_DEVICE_ID_JOYPAD_RIGHT,  SDL_SCANCODE_RIGHT  }, // D-Pad Right
    { RETRO_DEVICE_ID_JOYPAD_A,      SDL_SCANCODE_X      }, // Action A -> X key
    { RETRO_DEVICE_ID_JOYPAD_B,      SDL_SCANCODE_Z      }, // Action B -> Z key
    { RETRO_DEVICE_ID_JOYPAD_X,      SDL_SCANCODE_S      }, // Action X -> S key
    { RETRO_DEVICE_ID_JOYPAD_Y,      SDL_SCANCODE_A      }, // Action Y -> A key
    { RETRO_DEVICE_ID_JOYPAD_START,  SDL_SCANCODE_RETURN }, // Start    -> Enter
    { RETRO_DEVICE_ID_JOYPAD_SELECT, SDL_SCANCODE_SPACE  }, // Select   -> Space
    { RETRO_DEVICE_ID_JOYPAD_L,      SDL_SCANCODE_Q      }, // Left Shoulder
    { RETRO_DEVICE_ID_JOYPAD_R,      SDL_SCANCODE_W      }  // Right Shoulder
};

// 2. Hardware Controller Map (Maps Libretro buttons to SDL Game Controller Buttons)
const std::map<unsigned, SDL_GameControllerButton> g_gamepad_map = {
    { RETRO_DEVICE_ID_JOYPAD_UP,     SDL_CONTROLLER_BUTTON_DPAD_UP        },
    { RETRO_DEVICE_ID_JOYPAD_DOWN,   SDL_CONTROLLER_BUTTON_DPAD_DOWN      },
    { RETRO_DEVICE_ID_JOYPAD_LEFT,   SDL_CONTROLLER_BUTTON_DPAD_LEFT      },
    { RETRO_DEVICE_ID_JOYPAD_RIGHT,  SDL_CONTROLLER_BUTTON_DPAD_RIGHT     },
    { RETRO_DEVICE_ID_JOYPAD_A,      SDL_CONTROLLER_BUTTON_A              }, // Standard Xbox layout placement
    { RETRO_DEVICE_ID_JOYPAD_B,      SDL_CONTROLLER_BUTTON_B              },
    { RETRO_DEVICE_ID_JOYPAD_X,      SDL_CONTROLLER_BUTTON_X              },
    { RETRO_DEVICE_ID_JOYPAD_Y,      SDL_CONTROLLER_BUTTON_Y              },
    { RETRO_DEVICE_ID_JOYPAD_START,  SDL_CONTROLLER_BUTTON_START          },
    { RETRO_DEVICE_ID_JOYPAD_SELECT, SDL_CONTROLLER_BUTTON_BACK           },
    { RETRO_DEVICE_ID_JOYPAD_L,      SDL_CONTROLLER_BUTTON_LEFTSHOULDER   },
    { RETRO_DEVICE_ID_JOYPAD_R,      SDL_CONTROLLER_BUTTON_RIGHTSHOULDER  }
};

// Global core rom data
std::vector<char> g_rom_data_buffer;

// Global pointer tracking our active USB hardware device
SDL_GameController* g_gamepad = nullptr;

// --- Global Hardware States ---
bool        g_maintain_core_fps = true;
GLuint      g_core_texture = 0;
unsigned    g_pixel_format = RETRO_PIXEL_FORMAT_RGB565; // Default fallback layout
bool        g_core_supports_no_game = true; // Default


RetroLauncher::CRT g_crt;

// Structure to safely pass frame data from Core thread to Render thread
struct VideoBuffer {
    std::mutex mutex;
    std::vector<uint32_t> pixels; // Double buffer storage
    int width = 0;
    int height = 0;
    size_t pitch = 0;
    bool is_dirty = false;        // True when a brand new frame has arrived
} g_video_buffer;

std::atomic<bool> g_core_running{false};
std::atomic<bool> g_core_paused{false};

std::thread g_core_thread;

// --- Global Audio Tracking ---
SDL_AudioDeviceID g_audio_device = 0;

// --- Global Volume Modifiers ---
const float VOLUME_STEP = 0.05f;  // Increase/decrease by 5% increments

// --- Volume HUD Display States ---
uint32_t g_vol_hud_timeout_ms = 0;   // Timestamp when the overlay should vanish
char g_vol_hud_string[32] = "";      // Stores string like "VOL: 80%"
bool g_vol_hud_active = false;       // Flags whether the volume overlay is active

// --- Video Mode HUD Display States ---
uint32_t g_video_mode_hud_timeout_ms = 0; // Timestamp when the overlay should vanish
char g_video_mode_hud_string[32] = "";    // Stores string like "COMPONENT NTSC"
bool g_video_mode_hud_active = true;      // Flags whether the video mode overlay is active

// --- Audio Resampling Configuration States ---
double g_core_sample_rate = 44100.0; // Overwritten by av_info during boot
double g_current_resample_rate = 44100.0;

// High-grade low-pass tracking factor to prevent rapid audio pitch shifting
const double RESAMPLE_SMOOTH_FACTOR = 0.05; 

// Target audio buffer cushion (roughly 40ms of latency buffer)
// 48000Hz * 4 bytes per frame * 0.04s = ~7680 bytes
const uint32_t TARGET_BUFFER_BYTES = 7680;
const uint32_t MAX_BUFFER_BYTES = 16384; 

// --- Precision Timing States ---
double g_target_fps = 60.0;
uint64_t g_frame_duration_counts = 0; // Target duration mapped to performance counter ticks
uint64_t g_next_frame_time = 0;       // Monotonic timestamp when the next frame is due

// --- Complete Embedded 8x8 Font Bitmap Database (41 Characters) ---
// Sequence: '0'-'9' (0-9), '.' (10), ':' (11), ' ' (12), A-Z (13-38), '-' (39), '%' (40)
const uint8_t GL_HUD_FONT[41][8] = {
    {0x3C,0x66,0x6E,0x7E,0x76,0x66,0x3C,0x00}, // 0: '0'
    {0x18,0x18,0x38,0x18,0x18,0x18,0x7E,0x00}, // 1: '1'
    {0x3C,0x66,0x06,0x0C,0x30,0x60,0x7E,0x00}, // 2: '2'
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, // 3: '3'
    {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x1E,0x00}, // 4: '4'
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, // 5: '5'
    {0x3C,0x66,0x60,0x7C,0x66,0x66,0x3C,0x00}, // 6: '6'
    {0x7E,0x66,0x06,0x0C,0x18,0x18,0x18,0x00}, // 7: '7'
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, // 8: '8'
    {0x3C,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, // 9: '9'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // 10: '.'
    {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00}, // 11: ':'
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 12: ' ' (Space)
    
    // --- Capital Letters A - Z (Indices 13 to 38) ---
    {0x18,0x3C,0x66,0x7E,0x66,0x66,0x66,0x00}, // 13: 'A'
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, // 14: 'B'
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, // 15: 'C'
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, // 16: 'D'
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00}, // 17: 'E'
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00}, // 18: 'F'
    {0x3C,0x66,0x60,0x6C,0x66,0x66,0x3A,0x00}, // 19: 'G'
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // 20: 'H'
    {0x3E,0x0C,0x0C,0x0C,0x0C,0x0C,0x3E,0x00}, // 21: 'I'
    {0x1E,0x06,0x06,0x06,0x06,0x66,0x3C,0x00}, // 22: 'J'
    {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, // 23: 'K'
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, // 24: 'L'
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, // 25: 'M'
    {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00}, // 26: 'N'
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 27: 'O'
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, // 28: 'P'
    {0x3C,0x66,0x66,0x66,0x6E,0x6C,0x36,0x00}, // 29: 'Q'
    {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00}, // 30: 'R'
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, // 31: 'S'
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // 32: 'T'
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 33: 'U'
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, // 34: 'V'
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // 35: 'W'
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00}, // 36: 'X'
    {0x66,0x66,0x3C,0x18,0x18,0x18,0x18,0x00}, // 37: 'Y'
    {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}, // 38: 'Z'
    
    // --- Additional Trailing Utilities ---
    {0x00,0x00,0x00,0x3E,0x00,0x00,0x00,0x00}, // 39: '-' (Minus / Dash)
    {0x62,0x66,0x0C,0x18,0x30,0x64,0x46,0x00}  // 40: '%' (Percent)
};


// --- FPS Tracking States ---
uint32_t g_hud_frame_count = 0;
uint64_t g_hud_last_time = 0;
float g_hud_current_fps = 0.0f;
char g_hud_string[16] = "0.0 FPS";

// Maps an ASCII char to our local GL_HUD_FONT dictionary index
int get_font_index(char c) {
    switch (c) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case '.': return 10;
        case ':': return 11;
        case ' ': return 12;
        
        // Stack lowercase and uppercase to map to indices 13-38 smoothly
        case 'A': case 'a': return 13;
        case 'B': case 'b': return 14;
        case 'C': case 'c': return 15;
        case 'D': case 'd': return 16;
        case 'E': case 'e': return 17;
        case 'F': case 'f': return 18;
        case 'G': case 'g': return 19;
        case 'H': case 'h': return 20;
        case 'I': case 'i': return 21;
        case 'Y': case 'y': return 37; // Keep sequential order
        case 'J': case 'j': return 22;
        case 'K': case 'k': return 23;
        case 'L': case 'l': return 24;
        case 'M': case 'm': return 25;
        case 'N': case 'n': return 26;
        case 'O': case 'o': return 27;
        case 'P': case 'p': return 28;
        case 'Q': case 'q': return 29;
        case 'R': case 'r': return 30;
        case 'S': case 's': return 31;
        case 'T': case 't': return 32;
        case 'U': case 'u': return 33;
        case 'V': case 'v': return 34;
        case 'W': case 'w': return 35;
        case 'X': case 'x': return 36;
        case 'Z': case 'z': return 38;
        
        case '-': return 39;
        case '%': return 40;
        default:  return 12; // Fallback to safe blank empty space ' '
    }
}


std::string getVolumeBarString(float audio_volume, int vol_string_len) {
    if(audio_volume < 0.01) return "MUTED";
    if(vol_string_len < 2) return "VOLUME []";
    audio_volume = std::max(0.0f, std::min(1.0f, audio_volume));

    int inner_len = vol_string_len - 2;
    int filled_bars = static_cast<int>(audio_volume * inner_len);
    int empty_bars = inner_len - filled_bars;
    return "VOLUME [" + std::string(filled_bars, '|') + std::string(empty_bars, '-') + "]";
}

// Draws a raw monochrome bit-mapped pixel character string onto the display
void draw_hud_string(float start_x, float start_y, const char* text, float scale) {
    glUseProgram(0); // Temporarily suspend the shader layout architecture to draw flat
    glDisable(GL_TEXTURE_2D); // Crucial: Disable texturing so shapes render as flat colors
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity(); // Reset projection matrix to simple normalized device coordinates (-1.0 to 1.0)
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity(); // Reset modelview matrix to prevent game state scaling/rotation bleed

    // Loop through every text character in the string until the null terminator
    for (int i = 0; text[i] != '\0'; ++i) {
        int font_idx = get_font_index(text[i]);
        
        // Compute horizontal placement for the current character. 
        // We use 8 pixels of character width + 1 pixel of empty space padding.
        float char_offset_x = start_x + (i * 9.0f * scale); 

        // Loop through the 8 rows of the character bitmap matrix
        for (int row = 0; row < 8; ++row) {
            uint8_t byte = GL_HUD_FONT[font_idx][row];
            
            // Move downwards for subsequent rows in the character bitmap
            float py = start_y - (row * scale);

            // Loop through the 8 horizontal bits of this row (MSB to LSB)
            for (int col = 0; col < 8; ++col) {
                // If the bit at this position is active (1), draw a pixel block
                if (byte & (0x80 >> col)) { 
                    float px = char_offset_x + (col * scale);
                    
                    glBegin(GL_QUADS);
                        glVertex2f(px, py);
                        glVertex2f(px + scale, py);
                        glVertex2f(px + scale, py + scale);
                        glVertex2f(px, py + scale);
                    glEnd();
                }
            }
        }
    }

    // Restore previous OpenGL matrix states so we don't break game presentation
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    glEnable(GL_TEXTURE_2D); // Re-enable texturing for the next frame iteration
}

// Environment callback placeholder
bool cb_environment(unsigned cmd, void *data) {
    switch (cmd) {
        case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME: // cmd 18
            if (data) {
                g_core_supports_no_game = *(const bool*)data;
            }
            return true;
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
            if (data) {
                unsigned requested_format = *(const unsigned*)data;

                std::cout << "Core requested format " << requested_format << std::endl;
                
                // Explicitly check if the core wants XRGB8888, RGB565, or 0RGB1555
                if (requested_format == RETRO_PIXEL_FORMAT_XRGB8888 ||
                    requested_format == RETRO_PIXEL_FORMAT_RGB565   ||
                    requested_format == RETRO_PIXEL_FORMAT_0RGB1555) {
                    
                    // Assign the layout to our global state variable
                    g_pixel_format = requested_format;
                    
                    // Return TRUE to tell the core: "Yes, I support this layout!"
                    return true;
                }
            }
            return false;
        case RETRO_ENVIRONMENT_GET_CAN_DUPE:
            if (data) *(bool*)data = true;
            return true;
        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
            if (data) {
                // Core provides a pointer to a struct. We fill its function pointer.
                auto *cb = (struct retro_log_callback*)data;
                cb->log = cb_log_printf;
                return true;
            }
            return false;
        default:
            return false;
    }
}

// Video refresh callback: updates the OpenGL texture
void cb_video_refresh(const void *data, unsigned width, unsigned height, size_t pitch) {
    if (!data) return;
    
    std::lock_guard<std::mutex> lock(g_video_buffer.mutex);
    g_video_buffer.width = width;
    g_video_buffer.height = height;
    g_video_buffer.pitch = pitch;

    // Resize vector if frame size changes (e.g., SNES hires mode changes)
    size_t total_bytes = height * pitch;
    if (g_video_buffer.pixels.size() < total_bytes / 4) {
        g_video_buffer.pixels.resize(total_bytes / 4);
    }

    std::memcpy(g_video_buffer.pixels.data(), data, total_bytes);
    g_video_buffer.is_dirty = true; // Signal main thread that a new frame is ready
}

// --- Core Audio Callbacks ---
// 1. Single sample callback (legacy, rarely triggered by modern cores)
void cb_audio_sample(int16_t left, int16_t right) {
    int16_t frame[2] = { left, right };
    if (g_audio_device) {
        SDL_QueueAudio(g_audio_device, frame, sizeof(frame));
    }
}

// 2. Batch sample callback (Modern performance default)
size_t cb_audio_batch(const int16_t *data, size_t frames) {
    if (!data || frames == 0) return 0;
    
    if (g_audio_device) {
        // Compute total byte footprint (Stereo 16-bit interleaved = 4 bytes per frame)
        size_t byte_size = frames * 2 * sizeof(int16_t);
        
        // Push raw sample payload straight to hardware queue buffer
        SDL_QueueAudio(g_audio_device, data, byte_size);
    }
    return frames;
}

// Explicit batch sample callback with audio buffer overflow regulation
size_t cb_audio_batch_cb(const int16_t *data, size_t frames) {
    if (!data || frames == 0 || g_audio_device == 0) return 0;

    // 1. Measure real-time hardware buffer deviation
    uint32_t currently_queued = SDL_GetQueuedAudioSize(g_audio_device);
    int32_t buffer_error = (int32_t)currently_queued - (int32_t)TARGET_BUFFER_BYTES;

    // 2. Adjust target frequency based on buffer drift (Math Inverted Here)
    // Positive error (too much data) -> lower drift_adjust -> lower target_rate -> reads faster
    // Negative error (too little data) -> higher drift_adjust -> higher target_rate -> reads slower
    double drift_adjust = 1.0 + (double)buffer_error / (MAX_BUFFER_BYTES * 20.0);
    drift_adjust = std::clamp(drift_adjust, 0.995, 1.005);

    // Apply exponential smoothing to the sample rate adjustment
    double target_rate = g_core_sample_rate * drift_adjust;
    g_current_resample_rate = (g_current_resample_rate * (1.0 - RESAMPLE_SMOOTH_FACTOR)) + (target_rate * RESAMPLE_SMOOTH_FACTOR);

    // 3. Compute dynamic output frame count
    double resample_ratio = g_core_sample_rate / g_current_resample_rate;
    size_t out_frames = (size_t)(frames / resample_ratio);
    if (out_frames == 0) return frames;

    // Temporary storage vector for the resampled stereo stream
    std::vector<int16_t> output_buffer(out_frames * 2);

    // 4. Linear Interpolation Resampling Loop
    for (size_t i = 0; i < out_frames; ++i) {
        double src_index = i * resample_ratio;
        size_t index_low = (size_t)src_index;
        size_t index_high = index_low + 1;

        // Clip constraints to avoid memory boundary overflows
        if (index_high >= frames) {
            index_high = index_low;
        }

        double weight_high = src_index - index_low;
        double weight_low = 1.0 - weight_high;

        // Process Left Channel
        int16_t s0_l = data[index_low * 2];
        int16_t s1_l = data[index_high * 2];
        output_buffer[i * 2] = (int16_t)(s0_l * weight_low + s1_l * weight_high);

        // Process Right Channel
        int16_t s0_r = data[index_low * 2 + 1];
        int16_t s1_r = data[index_high * 2 + 1];
        output_buffer[i * 2 + 1] = (int16_t)(s0_r * weight_low + s1_r * weight_high);
    }

    // --- 4b. MULTIPLY SAMPLES BY GLOBAL VOLUME COEFFICIENT ---
    if (g_settings.audio_volume != 1.0f) {
        for (size_t i = 0; i < output_buffer.size(); ++i) {
            // Apply volume scale factor as higher precision float math
            float scaled_sample = (float)output_buffer[i] * g_settings.audio_volume;

            // Strict hardware boundaries clamping to prevent audio wrapping distortion
            if (scaled_sample > 32767.0f)  scaled_sample = 32767.0f;
            if (scaled_sample < -32768.0f) scaled_sample = -32768.0f;

            output_buffer[i] = (int16_t)scaled_sample;
        }
    }

    // 5. Hard protection boundary against absolute hardware blocking
    if (currently_queued < MAX_BUFFER_BYTES) {
        SDL_QueueAudio(g_audio_device, output_buffer.data(), output_buffer.size() * sizeof(int16_t));
    }

    return frames;
}


// 1. Core calls this to tell the frontend to read device hardware inputs
void cb_input_poll() {
    // We handle system events via SDL_PollEvent in the main loop instead.
    // This function can remain blank for keyboard states as long as SDL_PumpEvents updates under the hood.
}

// 2. Core calls this to check if a specific button is currently pressed
int16_t cb_input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
    if (port != 0) return 0;

    if (device == RETRO_DEVICE_KEYBOARD) {
        const Uint8* kbd_state = SDL_GetKeyboardState(NULL);

        // Scan through our map to find which physical SDL scancode corresponds to the core's requested ID
        for (const auto& pair : g_raw_keyboard_map) {
            if (pair.second == id) {
                SDL_Scancode scancode = pair.first;
                return kbd_state[scancode] ? 1 : 0;
            }
        }
        return 0;
    }


    // A. HANDLE ANALOG AXIS STICKS
    if (device == RETRO_DEVICE_ANALOG) {
        if (!g_gamepad) return 0;

        SDL_GameControllerAxis target_axis = SDL_CONTROLLER_AXIS_INVALID;

        // Determine left vs right thumbstick request
        if (index == RETRO_DEVICE_INDEX_ANALOG_LEFT) {
            target_axis = (id == RETRO_DEVICE_ID_ANALOG_X) ? SDL_CONTROLLER_AXIS_LEFTX : SDL_CONTROLLER_AXIS_LEFTY;
        } else if (index == RETRO_DEVICE_INDEX_ANALOG_RIGHT) {
            target_axis = (id == RETRO_DEVICE_ID_ANALOG_X) ? SDL_CONTROLLER_AXIS_RIGHTX : SDL_CONTROLLER_AXIS_RIGHTY;
        }

        if (target_axis != SDL_CONTROLLER_AXIS_INVALID) {
            int16_t raw_value = SDL_GameControllerGetAxis(g_gamepad, target_axis);

            // Apply radial/linear deadzone filter to kill jittery stick-drift
            if (std::abs(raw_value) > ANALOG_DEADZONE) {
                return raw_value; 
            }
        }
        return 0;
    }

    // B. HANDLE STANDARD DIGITAL JOYPAD BUTTONS
    if (device == RETRO_DEVICE_JOYPAD) {
        if (g_gamepad) {
            auto it = g_gamepad_map.find(id);
            if (it != g_gamepad_map.end() && SDL_GameControllerGetButton(g_gamepad, it->second)) {
                return 1;
            }
        }

        // Keyboard mapping fallback
        const Uint8* kbd_state = SDL_GetKeyboardState(NULL);
        auto it = g_key_map.find(id);
        if (it != g_key_map.end() && kbd_state[it->second]) {
            return 1;
        }
    }

    return 0;
}

// --- Controller Lifecycle Helpers ---
void init_gamepad_subsystem() {
    // Ensure the GameController subsystem is booted up
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "Failed to init GameController: " << SDL_GetError() << std::endl;
        return;
    }

    // Query existing devices already connected to your computer
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            g_gamepad = SDL_GameControllerOpen(i);
            if (g_gamepad) {
                std::cout << "Connected Controller: " << SDL_GameControllerName(g_gamepad) << std::endl;
                break; // Grab the first available controller for Player 1
            }
        }
    }
}

void close_gamepad_subsystem() {
    if (g_gamepad) {
        SDL_GameControllerClose(g_gamepad);
        g_gamepad = nullptr;
    }
}

retro_run_t global_retro_run = nullptr; 

void core_thread_loop() {
    // Precise timing setup for target FPS (e.g., 60 FPS = 16.666ms per frame)
    const uint64_t target_frame_time_ns = static_cast<uint64_t>(1'000'000'000.0 / g_target_fps);
    
    auto last_time = std::chrono::high_resolution_clock::now();

    while (g_core_running) {
        if (g_core_paused) {
            // Sleep for a short duration (e.g., 10ms) so the thread idle-loops 
            // without consuming 100% of a CPU core.
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Reset the timing clock so that when we unpause, the core doesn't 
            // experience a massive delta-time jump and try to fast-forward.
            last_time = std::chrono::high_resolution_clock::now();
            continue; 
        }

        // 1. Process Emulation Step
        global_retro_run(); // This fires cb_video_refresh internally

        // 2. Core Throttling 
        // Note: If you implement blocking audio sync (e.g., SDL_QueueAudio blocking),
        // the audio system will naturally throttle this loop, and you can skip this timer.
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - last_time).count();
        
        if (elapsed_ns < target_frame_time_ns) {
            uint64_t sleep_ns = target_frame_time_ns - elapsed_ns;
            std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_ns));
        }
        last_time = std::chrono::high_resolution_clock::now();
    }
}

RetroLauncher::OSD* g_osd_ptr = nullptr;

int main(int argc, char *argv[]) {
    loadINI(g_settings);

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_core.so> (<optional_path_to_rom>)" << std::endl;
        return 1;
    }

    const char* core_path = argv[1];
    const char* rom_path = (argc > 2) ? argv[2] : nullptr;

    // 1. Load the Libretro Core Dynamic Library
    void* core_handle = DLOPEN(core_path);
    if (!core_handle) {
        std::cerr << "Failed to load core: " << DLERROR() << std::endl;
        return 1;
    }

    // Resolve required libretro functions
    auto retro_init = (retro_init_t)DLSYM(core_handle, "retro_init");
    auto retro_deinit = (retro_deinit_t)DLSYM(core_handle, "retro_deinit");
    auto retro_api_version = (retro_api_version_t)DLSYM(core_handle, "retro_api_version");
    auto retro_get_system_info = (retro_get_system_info_t)DLSYM(core_handle, "retro_get_system_info");
    auto retro_get_system_av_info = (retro_get_system_av_info_t)DLSYM(core_handle, "retro_get_system_av_info");
    auto retro_set_environment = (retro_set_environment_t)DLSYM(core_handle, "retro_set_environment");
    auto retro_set_video_refresh = (retro_set_video_refresh_t)DLSYM(core_handle, "retro_set_video_refresh");
    auto retro_set_audio_sample = (retro_set_audio_sample_t)DLSYM(core_handle, "retro_set_audio_sample");
    auto retro_set_audio_sample_batch = (retro_set_audio_sample_batch_t)DLSYM(core_handle, "retro_set_audio_sample_batch");
    auto retro_set_input_poll = (retro_set_input_poll_t)DLSYM(core_handle, "retro_set_input_poll");
    auto retro_set_input_state = (retro_set_input_state_t)DLSYM(core_handle, "retro_set_input_state");
    auto retro_load_game = (retro_load_game_t)DLSYM(core_handle, "retro_load_game");
    auto retro_unload_game = (retro_unload_game_t)DLSYM(core_handle, "retro_unload_game");
    
    //auto retro_run = (retro_run_t)DLSYM(core_handle, "retro_run");
    global_retro_run = (retro_run_t)DLSYM(core_handle, "retro_run");

     if (!global_retro_run) {
        std::cerr << "Failed to find retro_run symbol!" << std::endl;
        return -1;
    }

    if (retro_api_version() != RETRO_API_VERSION) {
        std::cerr << "Libretro API version mismatch!" << std::endl;
        return 1;
    }

    // Connect core callbacks
    retro_set_environment(cb_environment);
    retro_set_video_refresh(cb_video_refresh);
    retro_set_audio_sample(cb_audio_sample);
    retro_set_audio_sample_batch(cb_audio_batch_cb);
    retro_set_input_poll(cb_input_poll);
    retro_set_input_state(cb_input_state);

    retro_init();
    retro_system_info sys_info;
    retro_get_system_info(&sys_info);

    std::cout << "Core Name: " << (sys_info.library_name ? sys_info.library_name : "Unknown") << "\n";
    std::cout << "Core Version: " << (sys_info.library_version ? sys_info.library_version : "Unknown") << "\n";
    std::cout << "Storage Strategy: " << (sys_info.need_fullpath ? "Requires Full Path" : "Can Load From Memory") << "\n";
    std::cout << "Block ZIP Extract: " << (sys_info.block_extract ? "Yes" : "No") << "\n";
    

    if (sys_info.valid_extensions) {
        std::cout << "Supported Extensions: ";
        std::string ext_str(sys_info.valid_extensions);
        std::stringstream ss(ext_str);
        std::string ext;
        
        while (std::getline(ss, ext, '|')) {
            std::cout << "." << ext << " ";
        }
        std::cout << "\n";
    }


    // 2. Load the Game Rom
    retro_game_info game_info = {rom_path, nullptr, 0, nullptr};
    if(rom_path) {
        game_info.path = rom_path;

        if (!sys_info.need_fullpath) {
            std::ifstream file(std::string(rom_path), std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                std::cerr << "Failed to open ROM: " << rom_path << std::endl;
                return false;
            }

            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            // Allocate memory and read the file
            g_rom_data_buffer.resize(size);
            if (!file.read(g_rom_data_buffer.data(), size)) {
                std::cerr << "Failed to read ROM data" << std::endl;
                return false;
            }

            // Populate memory fields
            game_info.data = g_rom_data_buffer.data();
            game_info.size = g_rom_data_buffer.size();
            
            std::cout << "Loaded ROM into memory. Size: " << size << " bytes.\n";
        } else {
            std::cout << "Passing ROM path directly to core: " << rom_path << "\n";
        }
    }

    if (!retro_load_game(&game_info)) {
        std::cerr << "Failed to load ROM: " << rom_path << std::endl;
        return 1;
    }

    retro_system_av_info av_info;
    retro_get_system_av_info(&av_info);

    // Initialize your master resampler clocks with parameters dictated by the core
    g_core_sample_rate = av_info.timing.sample_rate;
    g_current_resample_rate = av_info.timing.sample_rate;
    g_target_fps = av_info.timing.fps;

    // 2. Fetch system clock frequency (ticks per second)
    uint64_t perf_frequency = SDL_GetPerformanceFrequency();

    // 3. Compute how many clock ticks must elapse per single frame execution
    g_frame_duration_counts = (uint64_t)((double)perf_frequency / g_target_fps);

    // 4. Set our baseline starting clock anchor
    g_next_frame_time = SDL_GetPerformanceCounter();

    // 3. Initialize SDL2 & OpenGL Window
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "SDL Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Request Modern OpenGL compatibility profile settings
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

    init_gamepad_subsystem(); // Boot up input layer

    int win_w = 1280, win_h = 720;
    SDL_Window* window = SDL_CreateWindow(
        "Retro Launcher",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_w, win_h,
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP
    );

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // Enable(1) / Disable(0) VSync
    SDL_GetWindowSize(window, &win_w, &win_h);


    // 1. Initialize core ImGui contexts
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Allow navigating layout menus with keyboard keys

    // OPTIONAL: Explicitly name or path your ini file (defaults to "imgui.ini")
    io.IniFilename = "launcher.ini"; 

    // 2. Choose system style profile
    ImGui::StyleColorsDark();

    // 3. Mount pipeline wrappers
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Dynamic SDL Audio Queue Configuration
    SDL_AudioSpec target_spec;
    SDL_zero(target_spec);

    // Core timing dictates target output frequency (e.g. 44100 or 48000 Hz)
    target_spec.freq     = (int)av_info.timing.sample_rate; 
    target_spec.format   = AUDIO_S16SYS; // Signed 16-bit native endian system audio
    target_spec.channels = 2;            // Standard Stereo channels
    target_spec.samples  = 1024;         // Low latency hardware chunk partition size
    target_spec.callback = nullptr;      // Crucial: Set to null to explicitly use the Queue API

    g_audio_device = SDL_OpenAudioDevice(nullptr, 0, &target_spec, nullptr, 0);
    if (g_audio_device == 0) {
        std::cerr << "Audio device fallback failure: " << SDL_GetError() << std::endl;
    } else {
        SDL_PauseAudioDevice(g_audio_device, 0); // Unpause hardware stream loop
    }

    // Active gamepad device tracking query
    if (SDL_NumJoysticks() > 0 && SDL_IsGameController(0)) {
        g_gamepad = SDL_GameControllerOpen(0);
    }

    // 4. Setup OpenGL Texture
    glGenTextures(1, &g_core_texture);
    glBindTexture(GL_TEXTURE_2D, g_core_texture);

    // Pre-allocate a large chunk of GPU memory (e.g., 1024x1024 or 2048x2048 max possible core size)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 1024, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // --- Boot and Configure Modern Pipeline Crt Shaders Processor ---
    if(!g_crt.init(win_w, win_h)) {
        std::cerr << "[Launcher] Error initializing " << g_crt.getDisplayName() << " display !" << std::endl;
        return 1;
    }

    g_osd_ptr = g_crt.getOSD();


    // Start Core Thread
    g_core_running = true;
    g_core_thread = std::thread(core_thread_loop);

    std::vector<uint32_t> render_pixels;
    int tex_w = 0, tex_h = 0;
    size_t tex_pitch = 0;


    // 5. Main Execution Loop
    bool running = true;

    size_t frontend_frame_count = 0;

    while (running) {
        // Explicitly processes the operating system's message queue and updates keyboard arrays
        SDL_PumpEvents();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT || 
               (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }

            bool volume_changed = false;
            bool video_mode_changed = false;

            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_F3:
                        g_settings.show_fps = !g_settings.show_fps;
                        break;
                    case SDLK_F11:
                        g_settings.imgui_hud_show = !g_settings.imgui_hud_show;
                    case SDLK_F4:
                        g_maintain_core_fps = !g_maintain_core_fps;
                        break;
                    case SDLK_p:
                        g_core_paused = !g_core_paused;
                        break;
                    case SDLK_s: 
                        g_settings.use_shaders = !g_settings.use_shaders;
                        break;
                    case SDLK_v:
                        {
                            uint8_t current_video_mode = (uint8_t)g_crt.getMode();
                            g_crt.setMode((RetroLauncher::CRT::Mode)(++current_video_mode));
                        }
                        video_mode_changed = true;
                        break;
                    case SDLK_t:
                        {
                            uint8_t current_video_standard = (uint8_t)g_crt.getStandard();
                            g_crt.setStandard((RetroLauncher::CRT::Standard)(++current_video_standard));
                        }
                        video_mode_changed = true;
                        break;
                    case SDLK_MINUS:
                    case SDLK_KP_MINUS:
                        g_settings.audio_volume = std::max(0.0f, g_settings.audio_volume - VOLUME_STEP);
                        volume_changed = true;
                        break;
                    case SDLK_EQUALS:
                    case SDLK_PLUS:
                    case SDLK_KP_PLUS:
                        g_settings.audio_volume = std::min(2.0f, g_settings.audio_volume + VOLUME_STEP); // Boost up to 200%
                        volume_changed = true;
                        break;

                }
            }

            // Video Mode Change Processing Logic
            if(video_mode_changed) {
                // Compile current gain status to text array
                std::string video_mode_string = g_crt.getModeString();
                snprintf(g_video_mode_hud_string, sizeof(g_video_mode_hud_string), "%s", video_mode_string.c_str());
                
                // Set visible duration frame to 2000ms from right now
                g_video_mode_hud_timeout_ms = SDL_GetTicks() + 2000;
                g_video_mode_hud_active = true;
                
                std::cout << "[VIDEO MODE] " << video_mode_string << std::endl;
            }

            // Volume Hotkey Processing Logic (+ and -)
            if (volume_changed) {
                // Compile current gain status to text array
                snprintf(g_vol_hud_string, sizeof(g_vol_hud_string), "VOL: %d%%", (int)(g_settings.audio_volume * 100.0f));
                
                // Set visible duration frame to 2000ms from right now
                g_vol_hud_timeout_ms = SDL_GetTicks() + 2000;
                g_vol_hud_active = true;
                
                std::cout << "[AUDIO] " << g_vol_hud_string << std::endl;
            }
            
            // --- Track Hotplug Events Live ---
            if (event.type == SDL_CONTROLLERDEVICEADDED) {
                if (!g_gamepad) { // If player 1 doesn't have a controller mapped yet
                    g_gamepad = SDL_GameControllerOpen(event.cdevice.which);
                    std::cout << "Gamepad connected: " << SDL_GameControllerName(g_gamepad) << std::endl;
                }
            }
            else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
                if (g_gamepad) {
                    // Fetch structural instance hardware target key ID
                    SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(g_gamepad));
                    if (event.cdevice.which == id) {
                        SDL_GameControllerClose(g_gamepad);
                        g_gamepad = nullptr;
                        std::cout << "Gamepad disconnected. Falling back to keyboard." << std::endl;
                    }
                }
            }
        }

        // 1. Thread Synchronization Check
        size_t tex_pitch = 0;
        bool upload_new_frame = false;
        {
            std::lock_guard<std::mutex> lock(g_video_buffer.mutex);
            if (g_video_buffer.is_dirty) {
                // Quickly swap or copy data into render-thread local space 
                // to minimize mutex holding time.
                render_pixels = g_video_buffer.pixels;
                tex_w = g_video_buffer.width;
                tex_h = g_video_buffer.height;
                tex_pitch = g_video_buffer.pitch;
                
                g_video_buffer.is_dirty = false; // Reset flag
                upload_new_frame = true;
            }
        }

        if (upload_new_frame) {
            glBindTexture(GL_TEXTURE_2D, g_core_texture);
            GLint internal_format = GL_RGB;
            GLenum gl_type = GL_UNSIGNED_SHORT_5_6_5;
            GLenum gl_format = GL_RGB;
            size_t pixel_bytes = 2;

            // Dynamically query environmental layout agreements
            switch (g_pixel_format) {
                case RETRO_PIXEL_FORMAT_XRGB8888:
                    internal_format = GL_RGB;
                    gl_type = GL_UNSIGNED_BYTE;
                    gl_format = GL_BGRA; // Matches 32-bit hardware layout order maps
                    pixel_bytes = 4;
                    break;
                case RETRO_PIXEL_FORMAT_0RGB1555:
                    internal_format = GL_RGB;
                    gl_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
                    gl_format = GL_BGRA;
                    pixel_bytes = 2;
                    break;
                case RETRO_PIXEL_FORMAT_RGB565:
                default:
                    internal_format = GL_RGB;
                    gl_type = GL_UNSIGNED_SHORT_5_6_5;
                    gl_format = GL_RGB;
                    pixel_bytes = 2;
                    break;
            }

            glPixelStorei(GL_UNPACK_ROW_LENGTH, tex_pitch / pixel_bytes);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 
                0, 0,                // Offset X, Y
                tex_w, tex_h,        // Width and Height of the incoming core frame
                GL_BGRA, GL_UNSIGNED_BYTE, 
                render_pixels.data());
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        
        g_crt.setFrontendFrameCount(frontend_frame_count);
        bool legacy_render = !g_settings.use_shaders;
        if(g_settings.use_shaders) {
            legacy_render = !g_crt.process(g_core_texture, tex_w, tex_h);
        } 

        glViewport(0, 0, win_w, win_h);
        if(legacy_render){
            // Render the frame onto screen via OpenGL
            if (tex_w > 0 && tex_h > 0) {
                glEnable(GL_TEXTURE_2D);
                glUseProgram(0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, g_core_texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

                float max_u = static_cast<float>(tex_w) / 1024.0f;
                float max_v = static_cast<float>(tex_h) / 1024.0f;

                glBegin(GL_QUADS);
                    glColor3f(1.0f, 1.0f, 1.0f); 
                    glTexCoord2f(0.0f,   0.0f); glVertex2f(-1.0f,  1.0f);
                    glTexCoord2f(0.0f,  max_v); glVertex2f(-1.0f, -1.0f);
                    glTexCoord2f(max_u, max_v); glVertex2f( 1.0f, -1.0f);
                    glTexCoord2f(max_u,  0.0f); glVertex2f( 1.0f,  1.0f);
                glEnd();
               
                glBindTexture(GL_TEXTURE_2D, 0);
                glDisable(GL_TEXTURE_2D); // Crucial: Disable texturing so shapes render as flat colors
            }
        }

        // RENDER HUD OVERLAY (IF ENABLED)

        // --- 1. DYNAMIC FPS CALCULATION ---
        g_hud_frame_count++;
        uint64_t hud_now = SDL_GetPerformanceCounter();
        uint64_t hud_elapsed_ticks = hud_now - g_hud_last_time;
        double hud_elapsed_sec = (double)hud_elapsed_ticks / (double)SDL_GetPerformanceFrequency();

        if (g_settings.show_fps) {
            // Recalculate and update the text string once every 0.5 seconds to keep it readable
            if (hud_elapsed_sec >= 0.5) {
                g_hud_current_fps = (float)((double)g_hud_frame_count / hud_elapsed_sec);
                snprintf(g_hud_string, sizeof(g_hud_string), "%.1f FPS", g_hud_current_fps);
                
                // Reset tracking states for the next window window slice
                g_hud_frame_count = 0;
                g_hud_last_time = hud_now;
            }

            // --- 2. RENDER THE HUD OVERLAY ---
            // Render a high-visibility text layout (Bright Green text with a soft shadow effect)
            float px_scale = 0.004f; // Controls text sizing relative to raw normalized screen bounds (-1.0 to 1.0)
            
            // Draw Drop Shadow (Offset Black text)
            glColor3f(0.0f, 0.0f, 0.0f);
            draw_hud_string(-0.95f + 0.002f, 0.90f - 0.002f, g_hud_string, px_scale);

            // Draw Main Text (Bright Neon Green)
            glColor3f(0.0f, 1.0f, 0.0f);
            draw_hud_string(-0.95f, 0.90f, g_hud_string, px_scale);
            
            // Reset color to default solid white so it doesn't tint the game frame
            glColor3f(1.0f, 1.0f, 1.0f);
        }

        // --- DYNAMIC VOLUME POP-UP RENDERING ---
        if (g_vol_hud_active) {
            // Check if our time allocation window has elapsed
            if (SDL_GetTicks() > g_vol_hud_timeout_ms) {
                g_vol_hud_active = false; // Gracefully shut off drawing pass
                if(g_osd_ptr) g_osd_ptr->clear();
            } else {
                float vol_x = 0.65f;  // Placed on the top right quadrant
                float vol_y = 0.90f;
                float scale = 0.004f;

                if(g_osd_ptr) {
                    int v_pos = g_osd_ptr->getHeight() - 28;
                    g_osd_ptr->setText(10, v_pos, getVolumeBarString(g_settings.audio_volume, 20), 0, 255, 0);
                } else {
                    // Draw Drop Shadow (Flat Black offset background)
                    glColor3f(0.0f, 0.0f, 0.0f);
                    draw_hud_string(vol_x + 0.002f, vol_y - 0.002f, g_vol_hud_string, scale);

                    // Draw Primary Text (High-contrast Cyan/Light Blue text)
                    glColor3f(0.0f, 1.0f, 1.0f);
                    draw_hud_string(vol_x, vol_y, g_vol_hud_string, scale);

                    // Reset color to default solid white so it doesn't tint the game frame
                    glColor3f(1.0f, 1.0f, 1.0f);
                }
            }
        }

        // --- DYNAMIC VIDEO MODE POP-UP RENDERING ---
        if (g_video_mode_hud_active) {
            // Check if our time allocation window has elapsed
            if (SDL_GetTicks() > g_video_mode_hud_timeout_ms) {
                g_video_mode_hud_active = false; // Gracefully shut off drawing pass
                if(g_osd_ptr) g_osd_ptr->clear();
            } else {
                float vol_x = 0.475f;
                float vol_y = 0.80f;
                float scale = 0.004f;

                if(g_osd_ptr) {
                    g_osd_ptr->setText(10, 10, g_video_mode_hud_string, 0, 255, 0, 255);
                } else {

                    // Draw Drop Shadow (Flat Black offset background)
                    glColor3f(0.0f, 0.0f, 0.0f);
                    draw_hud_string(vol_x + 0.002f, vol_y - 0.002f, g_video_mode_hud_string, scale);

                    // Draw Primary Text (High-contrast Cyan/Light Blue text)
                    glColor3f(0.0f, 1.0f, 1.0f);
                    draw_hud_string(vol_x, vol_y, g_video_mode_hud_string, scale);

                    // Reset color to default solid white so it doesn't tint the game frame
                    glColor3f(1.0f, 1.0f, 1.0f);
                }
            }
        }

        // ImGui
        if(g_settings.imgui_hud_show) {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame(); 
            
            // test
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Appearing);
            ImGui::SetNextWindowSizeConstraints(ImVec2(400.0f, 200.0f), ImVec2(FLT_MAX, FLT_MAX));

            
            // Open a named window context so window is never NULL
            ImGui::Begin("Frontend Dashboard", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            
            ImGui::Text("System Perf: %.1f FPS", g_hud_current_fps);
            ImGui::SliderFloat("Audio Gain", &g_settings.audio_volume, 0.0f, 2.0f, "%.2f");

            if(g_settings.use_shaders){
                g_crt.drawUI();
            }

            ImGui::End(); // <-- Safely closes the named window context


            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        }

        SDL_GL_SwapWindow(window);
        frontend_frame_count++;
    }

    // 6. Cleanup & Shutdown
    g_core_running = false;
    if (g_core_thread.joinable()) {
        g_core_thread.join();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    g_crt.destroy();
    glDeleteTextures(1, &g_core_texture);

    if (g_gamepad) SDL_GameControllerClose(g_gamepad);
    if (g_audio_device) {SDL_CloseAudioDevice(g_audio_device);}

    retro_unload_game();
    retro_deinit();
    
    DLCLOSE(core_handle);

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    saveINI(g_settings);
    return 0;
}
