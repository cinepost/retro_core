#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "libretro.h"
#include "framework/vdp_nes.h"


#define FRAMEBUFFER_WIDTH 448
#define FRAMEBUFFER_HEIGHT 252

static RetroCore::VDP_NES gRenderer(FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);
//static RetroCore::AudioProcessor<RetroCore::Platform::NES> gAPU(48000);
static struct retro_log_callback logging;
static retro_log_printf_t log_cb;

static void fallback_log(enum retro_log_level level, const char *fmt, ...) {
   (void)level;
   va_list va;
   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

void retro_init(void) {
   const bool result = gRenderer.init();
   assert(result);
   if(!result) {
      log_cb(RETRO_LOG_ERROR, "RetroCore::Renderer initialization failed !\n");
   }
}

void retro_deinit(void) {
   if(!gRenderer.deinit()) {
      log_cb(RETRO_LOG_ERROR, "RetroCore::Renderer de-initialization failed !\n");
   }
}

unsigned retro_api_version(void) {
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device) {
   log_cb(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);
}

void retro_get_system_info(struct retro_system_info *info) {
   memset(info, 0, sizeof(*info));
   info->library_name     = "TestCore";
   info->library_version  = "v1";
   info->need_fullpath    = false;
   info->valid_extensions = NULL; // Anything is fine, we don't care.
}

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

void retro_get_system_av_info(struct retro_system_av_info *info) {
   float aspect = static_cast<float>(FRAMEBUFFER_WIDTH) / static_cast<float>(FRAMEBUFFER_HEIGHT);

   info->timing = (struct retro_system_timing) {
      .fps = 60.0,
      .sample_rate = 0.0,
   };

   info->geometry = (struct retro_game_geometry) {
      .base_width   = FRAMEBUFFER_WIDTH,
      .base_height  = FRAMEBUFFER_HEIGHT,
      .max_width    = FRAMEBUFFER_WIDTH,
      .max_height   = FRAMEBUFFER_HEIGHT,
      .aspect_ratio = aspect,
   };

}

void retro_set_environment(retro_environment_t cb) {
   environ_cb = cb;

   bool no_content = true;
   cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

   if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging)) {
      log_cb = logging.log;
   } else {
      log_cb = fallback_log;
   }
}

void retro_set_audio_sample(retro_audio_sample_t cb) {
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb) {
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb) {
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb) {
   video_cb = cb;
}

static unsigned x_coord;
static unsigned y_coord;

void retro_reset(void) {
   gRenderer.reset();
}

static void update_input(void) {
   static const uint16_t sKbdMouseMovementIncrement = 1;
   input_poll_cb();

   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP)) {
      /* Stub */
   }

   int16_t mouse_delta_x = 0;
   int16_t mouse_delta_y = 0;

   // Query keyboard keys using libretro key IDs
   bool up    = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_UP);
   bool down  = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_DOWN);
   bool left  = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LEFT);
   bool right = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RIGHT);

   if (up)    mouse_delta_y -= sKbdMouseMovementIncrement;
   if (down)  mouse_delta_y += sKbdMouseMovementIncrement;
   if (left)  mouse_delta_x -= sKbdMouseMovementIncrement;
   if (right) mouse_delta_x += sKbdMouseMovementIncrement;

   // Query mouse
   mouse_delta_x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
   mouse_delta_y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
   bool mouse_left_pressed = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
   bool mouse_right_pressed = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);

   gRenderer.moveDebugCursor(mouse_delta_x, mouse_delta_y);
}

static void render(void) {
   /* Try rendering straight into VRAM if we can. */
   const uint8_t *buf = nullptr;
   uint32_t stride_bytes = gRenderer.getFramebufferStride();
   struct retro_framebuffer fb = {0};
   fb.width = gRenderer.getFramebufferWidth();
   fb.height = gRenderer.getFramebufferHeight();
   fb.access_flags = RETRO_MEMORY_ACCESS_WRITE;
   if (environ_cb(RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER, &fb) && fb.format == RETRO_PIXEL_FORMAT_XRGB8888) {
      stride_bytes = fb.pitch;
      gRenderer.clearFramebuffer(static_cast<uint8_t*>(fb.data), stride_bytes);
      buf = gRenderer.render(static_cast<uint8_t*>(fb.data), stride_bytes);
   } else {
      gRenderer.clearFramebuffer();
      buf = gRenderer.render();
   }

   assert(buf);
   video_cb(buf, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, stride_bytes);
}

static void check_variables(void) {
}

static void audio_callback(void) {
   audio_cb(0, 0);
}

void update_refresh_rate(double new_fps) {
   static double current_fps = 0.0;
   
   if(current_fps == new_fps) return;

   struct retro_system_av_info av_info;
   retro_get_system_av_info(&av_info);
   av_info.timing.fps = new_fps;

   if (environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &av_info)) {
      current_fps = new_fps;
      log_cb(RETRO_LOG_INFO, "Refresh rate updated to: %f\n", new_fps);
   }
}

void retro_run(void) {
   assert(gRenderer.isInitialized());
   update_refresh_rate(1000.0); // debug fps
   update_input();
   render();
   audio_callback();

   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated) {
      check_variables();
   }
}


bool retro_load_game(const struct retro_game_info *info) {
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
      log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
      return false;
   }

   struct retro_keyboard_callback cb;
   cb.callback = [](bool down, unsigned keycode, uint32_t character, uint16_t mod) {
      if (down) {
         // Example: Detect Ctrl + R
         if ((mod & RETROKMOD_CTRL) != 0 && keycode == RETROK_r) {
            gRenderer.setShowDebugInfoState(!gRenderer.getShowDebugInfoState());
         }
      }
   };

   // Register with the frontend
   if (!environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb)) {
      log_cb(RETRO_LOG_INFO, "Error setting frontend callback!\n");
   }

   check_variables();

   (void)info;
   return true;
}

void retro_unload_game(void) {
}

unsigned retro_get_region(void) {
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num) {
   if (type != 0x200) {
      return false;
   }
   
   if (num != 2) {
      return false;
   }

   return retro_load_game(NULL);
}

size_t retro_serialize_size(void) {
   return 2;
}

bool retro_serialize(void *data_, size_t size) {
   if (size < 2) {
      return false;
   }

   uint8_t *data = static_cast<uint8_t*>(data_);
   data[0] = x_coord;
   data[1] = y_coord;
   return true;
}

bool retro_unserialize(const void *data_, size_t size) {
   if (size < 2) {
      return false;
   }

   const uint8_t *data = static_cast<const uint8_t*>(data_);
   x_coord = data[0] & 31;
   y_coord = data[1] & 31;
   return true;
}

void *retro_get_memory_data(unsigned id) {
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id) {
   (void)id;
   return 0;
}

void retro_cheat_reset(void) {

}

void retro_cheat_set(unsigned index, bool enabled, const char *code) {
   (void)index;
   (void)enabled;
   (void)code;
}
