#pragma once

#include <fstream>

#include <SDL.h>

// Format of game's WAV files

constexpr int32_t WAV_FREQUENCY = 11025; // Hz
constexpr int32_t WAV_FORMAT = 2; // 2 bytes, 16 bits
constexpr int32_t WAV_CHANNELS = 2; // Stereo
constexpr int32_t WAV_SAMPLES = 256;

// TODO: Does audio data in a WAV always start in the same offset?

constexpr int32_t WAV_DATA_START_POSITION = 0x2c;

class Audio
{
private:
	static SDL_AudioDeviceID audioDeviceId;
	static std::ifstream currentAudioStream;
	static int32_t currentAudioStreamLegth;

public:
	static bool Initialize();
	static void Dispose();

	static bool LoadAudioFromWAV(const std::string baseDataPath, const std::string fileName);
	static void StopAudio();
	static void SetAudioPlaybackTime(const double elapsedTime);

	inline static bool IsInitialized() { return audioDeviceId > 0; }

private:
	static void AudioCallback(void* userdata, uint8_t* stream, int32_t len);
};