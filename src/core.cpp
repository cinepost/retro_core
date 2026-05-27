#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "libretro.h"

#include "framework/renderer.h"
#include "framework/sound_engine.h"
#include "framework/apu/apu.h"
#include "framework/apu/apu_nes.h"
#include "framework/apu/apu_psg.h"
#include "framework/apu/apu_ym2612.h"
#include "framework/ppu/ppu_nes.h"

#include <memory>

//#define FRAMEBUFFER_WIDTH 448
//#define FRAMEBUFFER_HEIGHT 256

#define FRAMEBUFFER_WIDTH 512
#define FRAMEBUFFER_HEIGHT 288

#define TARGET_FPS 600.0
#define TARGET_SAMPLE_RATE 44100.0


static constexpr RetroCore::FramebufferDims gResolution(FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);

static RetroCore::PPU::NesPPU<gResolution> gPPU0;
static RetroCore::Renderer<RetroCore::Platform::NES, gResolution, RetroCore::PPU::NesPPU<gResolution>> gRenderer(gPPU0);

static RetroCore::Sound::SoundEngine gSoundEngine;

//static RetroCore::APU::HybridApu<RetroCore::APU::NesApu, RetroCore::APU::Ym2612Apu, RetroCore::APU::PsgApu> gAPU{
//   RetroCore::APU::NesApu{},
//   RetroCore::APU::Ym2612Apu{},
//   RetroCore::APU::PsgApu{}
//};

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
   log_cb(RETRO_LOG_INFO, "RetroCore::Renderer initialization started.\n");
   
   const bool result = gRenderer.init();
   assert(result);

   // Init default font
   size_t font_bytes_count = 0;
   const uint8_t* pFontData = RetroCore::PPU::NesPPU_BASE::getDefaultFontData(font_bytes_count);
   assert(pFontData);
   assert(font_bytes_count > 0);
   gPPU0.pushTiles<0>(pFontData, font_bytes_count, 0 /* push starting at tileIndex */);

   if(!result) {
      log_cb(RETRO_LOG_ERROR, "RetroCore::Renderer initialization failed !\n");
   }

/*
   gSoundEngine.init(&gAPU, RetroCore::Sound::SoundEngine::Config{16, 32, 64});

   RetroCore::Sound::MidiToMusicTrackConfig midiConf;
   midiConf.channelRouting[0] = {RetroCore::APU::ApuComponent::NES, 0, 127, 0};   // MIDI Ch 0 -> NES Pulse1
   midiConf.channelRouting[1] = {RetroCore::APU::ApuComponent::NES, 1, 127, -32}; // MIDI Ch 1 -> NES Pulse2 (-64 pan -> -32 in our scale)
   midiConf.channelRouting[2] = {RetroCore::APU::ApuComponent::YM2612, 0, 100, 0}; // MIDI Ch 2 -> Genesis FM1
   midiConf.channelRouting[3] = {RetroCore::APU::ApuComponent::PSG, 0, 127, 0};   // MIDI Ch 3 -> PSG Ch A
   midiConf.autoMapTempo = true;

   RetroCore::Sound::MusicTrackData musicData;
   if (RetroCore::Sound::MidiConverter::convert("/mnt/misc_hdd/dev/retro_core/assets/midi_files/boss.mid", musicData, midiConf)) {
      // 4. Create & Play
      uint32_t trackId = gSoundEngine.createTrack(musicData);
      gSoundEngine.playTrack(trackId);
    
      // Optional: Override pan/volume after creation
      gSoundEngine.setTrackVolume(trackId, 0.8f);
      gSoundEngine.setTrackPan(trackId, 0.2f);
   } else {
      log_cb(RETRO_LOG_ERROR, "RetroCore::SoundEngine midi loading failed !\n");
   }
*/
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
      .fps = TARGET_FPS,
      .sample_rate = TARGET_SAMPLE_RATE,
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

   // Tell the frontend that this core can boot without content
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
   fb.width = gResolution.width;
   fb.height = gResolution.height;
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
//   assert(gRenderer.isInitialized());
   update_refresh_rate(TARGET_FPS); // debug fps
   update_input();
   render();

/*
   size_t frames = TARGET_SAMPLE_RATE / TARGET_FPS;

   gSoundEngine.writeRegister(0x4015, 0x01); // NES APU register
   gSoundEngine.writeRegister(0x28, 0x30);   // Genesis FM register
   gSoundEngine.writeRegister(0x00, 0x00);   // PSG Channel A period (Note: PSG uses 0x00-0x0F)

   gSoundEngine.update(1.0f / TARGET_FPS);
   static std::vector<float> audio_buf; 
   audio_buf.resize(frames*2);
   gSoundEngine.finalizeAudio(audio_buf.data(), frames);

   //audio_callback();

   // 3. Convert to int16 for libretro
   static std::vector<int16_t> int16_buf;
   if (int16_buf.size() != frames * 2) int16_buf.resize(frames * 2);
    
   for (size_t i = 0; i < frames * 2; ++i) {
      int16_t s = static_cast<int16_t>(audio_buf[i] * 32767.0f);
      //s = std::max(-32768, std::min(32767, s));
      //s = i;
      int16_buf[i] = s;
   }

   if (audio_batch_cb) {
      audio_batch_cb(int16_buf.data(), frames);
   }
*/
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
