#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

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

// --- Libretro Input Device and Button ID Constants ---
#define RETRO_DEVICE_JOYPAD             1
#define RETRO_DEVICE_ANALOG             2

// --- Libretro Analog Index/ID Identifiers ---
#define RETRO_DEVICE_INDEX_ANALOG_LEFT   0
#define RETRO_DEVICE_INDEX_ANALOG_RIGHT  1
#define RETRO_DEVICE_ID_ANALOG_X         0
#define RETRO_DEVICE_ID_ANALOG_Y         1

// --- Libretro Digital Button IDs ---
#define RETRO_DEVICE_ID_JOYPAD_B        0
#define RETRO_DEVICE_ID_JOYPAD_Y        1
#define RETRO_DEVICE_ID_JOYPAD_SELECT   2
#define RETRO_DEVICE_ID_JOYPAD_START    3
#define RETRO_DEVICE_ID_JOYPAD_UP       4
#define RETRO_DEVICE_ID_JOYPAD_DOWN     5
#define RETRO_DEVICE_ID_JOYPAD_LEFT     6
#define RETRO_DEVICE_ID_JOYPAD_RIGHT    7
#define RETRO_DEVICE_ID_JOYPAD_A        8
#define RETRO_DEVICE_ID_JOYPAD_X        9
#define RETRO_DEVICE_ID_JOYPAD_L       10
#define RETRO_DEVICE_ID_JOYPAD_R       11

// Libretro Log Levels
enum retro_log_level {
    RETRO_LOG_DEBUG = 0,
    RETRO_LOG_INFO,
    RETRO_LOG_WARN,
    RETRO_LOG_ERROR,
    RETRO_LOG_DUMMY = 0x7FFFFFFF
};

// Callback structure passed by the core
struct retro_log_callback {
    void (*log)(enum retro_log_level level, const char *fmt, ...);
};

// 1. The Frontend Logger function
void cb_log_printf(enum retro_log_level level, const char *fmt, ...) {
    const char *level_str = "DEBUG";
    switch (level) {
        case RETRO_LOG_INFO:  level_str = "INFO";  break;
        case RETRO_LOG_WARN:  level_str = "WARN";  break;
        case RETRO_LOG_ERROR: level_str = "ERROR"; break;
        default: break;
    }

    printf("[%s] ", level_str);
    
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

// 1. Libretro Pixel Format Constant Defs (Add if missing)
#define RETRO_PIXEL_FORMAT_0RGB1555 0
#define RETRO_PIXEL_FORMAT_XRGB8888 1
#define RETRO_PIXEL_FORMAT_RGB565   2

#define RETRO_ENVIRONMENT_SET_ROTATION          1
#define RETRO_ENVIRONMENT_GET_CAN_DUPE          3
#define RETRO_ENVIRONMENT_GET_LOG_INTERFACE     4
#define RETRO_ENVIRONMENT_SET_PIXEL_FORMAT      10
#define RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME   18

// Minimal Libretro API definitions
#define RETRO_API_VERSION 1
#define RETRO_DEVICE_DEFAULT 1

struct retro_system_info {
    const char *library_name;
    const char *library_version;
    const char *valid_extensions;
    bool need_fullpath;
    bool block_extract;
};

struct retro_game_geometry {
    unsigned base_width;
    unsigned base_height;
    unsigned max_width;
    unsigned max_height;
    float aspect_ratio;
};

struct retro_system_timing {
    double fps;
    double sample_rate;
};

struct retro_system_av_info {
    struct retro_game_geometry geometry;
    struct retro_system_timing timing;
};

struct retro_game_info {
    const char *path;
    const void *data;
    size_t size;
    const char *meta;
};

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
typedef void (*retro_unload_game_t)(void);
typedef void (*retro_run_t)(void);

// Adjustable Controller Deadzone (SDL axes report values between -32768 and 32767)
const int16_t ANALOG_DEADZONE = 4000;

// Global input map translating Libretro button IDs to SDL Scancodes
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

// Global pointer tracking our active USB hardware device
SDL_GameController* g_gamepad = nullptr;

// --- Global Hardware States ---
GLuint g_texture = 0;
unsigned g_tex_width = 0;
unsigned g_tex_height = 0;
unsigned g_pixel_format = RETRO_PIXEL_FORMAT_RGB565; // Default fallback layout
bool     g_core_supports_no_game = true; // Default

// --- Global Audio Tracking ---
SDL_AudioDeviceID g_audio_device = 0;

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

// --- Minimalist Embedded 8x8 Font Bitmap Database ---
// Supports characters: '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', 'F', 'P', 'S', ' '
const uint8_t GL_HUD_FONT[15][8] = {
    {0x3C,0x66,0x6E,0x7E,0x76,0x66,0x3C,0x00}, // '0'
    {0x18,0x18,0x38,0x18,0x18,0x18,0x7E,0x00}, // '1'
    {0x3C,0x66,0x06,0x0C,0x30,0x60,0x7E,0x00}, // '2'
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, // '3'
    {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x1E,0x00}, // '4'
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, // '5'
    {0x3C,0x66,0x60,0x7C,0x66,0x66,0x3C,0x00}, // '6'
    {0x7E,0x66,0x06,0x0C,0x18,0x18,0x18,0x00}, // '7'
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, // '8'
    {0x3C,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, // '9'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // '.'
    {0x7E,0x60,0x7C,0x60,0x60,0x60,0xF0,0x00}, // 'F'
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00}, // 'P'
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, // 'S'
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}  // ' '
};

// --- FPS Tracking States ---
bool g_hud_visible = true; // Set to true by default
uint32_t g_hud_frame_count = 0;
uint64_t g_hud_last_time = 0;
float g_hud_current_fps = 0.0f;
char g_hud_string[16] = "0.0 FPS";

// Maps an ASCII char to our local GL_HUD_FONT dictionary index
int get_font_index(char c) {
    if (c >= '0' && c <= '9') return c - '0'; // Indices 0 to 9
    if (c == '.') return 10;                  // Index 10
    if (c == 'F' || c == 'f') return 11;      // Index 11
    if (c == 'P' || c == 'p') return 12;      // Index 12
    if (c == 'S' || c == 's') return 13;      // Index 13
    return 14;                                // Index 14 (Space / Fallback)
}

// Draws a raw monochrome bit-mapped pixel character string onto the display
void draw_hud_string(float start_x, float start_y, const char* text, float scale) {
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
    
    glBindTexture(GL_TEXTURE_2D, g_texture);
    g_tex_width = width;
    g_tex_height = height;

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

    glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / pixel_bytes);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, gl_format, gl_type, data);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
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

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <path_to_core.so> <path_to_rom>" << std::endl;
        return 1;
    }

    const char* core_path = argv[1];
    const char* rom_path = argv[2];

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
    auto retro_run = (retro_run_t)DLSYM(core_handle, "retro_run");

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

    // 2. Load the Game Rom
    retro_game_info game_info = { rom_path, nullptr, 0, nullptr };
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

    printf("fps %f\n", g_target_fps);

    // 3. Initialize SDL2 & OpenGL Window
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    init_gamepad_subsystem(); // Boot up input layer

    SDL_Window* window = SDL_CreateWindow(
        "Simple Libretro Frontend",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP
    );

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(0); // Enable VSync

    // --- Dynamic SDL Audio Queue Configuration ---
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

    // 4. Setup OpenGL Texture
    glGenTextures(1, &g_texture);
    glBindTexture(GL_TEXTURE_2D, g_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // 5. Main Execution Loop
    bool running = true;
    SDL_Event event;

    while (running) {
        // Explicitly processes the operating system's message queue and updates keyboard arrays
        SDL_PumpEvents();

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || 
               (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }
            // --- HUD Toggle Hotkey Detection ---
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F3) {
                g_hud_visible = !g_hud_visible; // Invert the visibility state
            }

            // --- Track Hotplug Events Live ---
            else if (event.type == SDL_CONTROLLERDEVICEADDED) {
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

        // --- HIGH PRECISION FRAMERATE TIMING STEP ---
        uint64_t current_time = SDL_GetPerformanceCounter();

        // Check if we are running ahead of the core's native target timeline
        if (current_time < g_next_frame_time) {
            uint64_t ticks_to_wait = g_next_frame_time - current_time;
            double ms_to_wait = ((double)ticks_to_wait * 1000.0) / (double)SDL_GetPerformanceFrequency();

            // 1. Coarse Sleep: If we have plenty of time (more than 2ms), yield to the OS
            if (ms_to_wait > 2.0) {
                SDL_Delay((Uint32)(ms_to_wait - 1.5)); 
            }

            // 2. Fine-Grained Busy Wait: Burn remaining sub-millisecond cycles until the target tick hits
            while (SDL_GetPerformanceCounter() < g_next_frame_time) {
                #if defined(_MSC_VER) || defined(__MINGW32__)
                    _mm_pause(); // Intrinsic hint optimizing CPU power usage during spinlocks
                #elif defined(__i386__) || defined(__x86_64__)
                    __builtin_ia32_pause();
                #endif
            }
        }

        // Accumulate target step forward for the next iteration frame point
        g_next_frame_time += g_frame_duration_counts;

        // Hard catch-up guard: If the emulator dips or hitches significantly,
        // reset our anchor to the current clock time to prevent rapid frame-skipping cycles
        if (SDL_GetPerformanceCounter() > g_next_frame_time + (g_frame_duration_counts * 2)) {
            g_next_frame_time = SDL_GetPerformanceCounter() + g_frame_duration_counts;
        }

        // Run one frame of emulation
        retro_run();

        // Render the frame onto screen via OpenGL
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (g_tex_width > 0 && g_tex_height > 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, g_texture);

            glBegin(GL_QUADS);
                glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
                glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f, -1.0f);
                glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f,  1.0f);
                glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f,  1.0f);
            glEnd();
        }

        // --- RENDER HUD OVERLAY (IF ENABLED) ---
        if (g_hud_visible) {
            // --- 1. DYNAMIC FPS CALCULATION ---
            g_hud_frame_count++;
            uint64_t hud_now = SDL_GetPerformanceCounter();
            uint64_t hud_elapsed_ticks = hud_now - g_hud_last_time;
            double hud_elapsed_sec = (double)hud_elapsed_ticks / (double)SDL_GetPerformanceFrequency();

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
            
            // Reset structural color engine to default solid white so it doesn't tint the game frame
            glColor3f(1.0f, 1.0f, 1.0f);
        }
    
        SDL_GL_SwapWindow(window);
    }

    // 6. Cleanup & Shutdown
    
    if (g_audio_device) {SDL_CloseAudioDevice(g_audio_device);}

    glDeleteTextures(1, &g_texture);

    retro_unload_game();
    retro_deinit();
    
    DLCLOSE(core_handle);

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
