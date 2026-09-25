#ifndef __RETRO_CORE_FRAMEWORK_GAME_ENGINE_SOUND_PLAYER_H
#define __RETRO_CORE_FRAMEWORK_GAME_ENGINE_SOUND_PLAYER_H

#include "sound_engine.h"
#include "mp3_stream.h"

namespace RetroCore {

namespace GameEngine {

// Thin SoundEngine play-only wrapper to pass around
class SoundPlayer {
    public:
        SoundPlayer(const SoundEngine& soundEngine): mSoundEngine(soundEngine) {}

        void playSFX(std::unique_ptr<GameEngine::AudioSource> pSfxTrack) const {
            mSoundEngine.playSFX(std::move(pSfxTrack));
        }

        template<typename T>
        void playSFX(const T& sfxTrack) const {
            mSoundEngine.playSFX(std::make_unique<T>(sfxTrack));
        }

    private:
        const SoundEngine& mSoundEngine;
};

}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_SOUND_PLAYER_H