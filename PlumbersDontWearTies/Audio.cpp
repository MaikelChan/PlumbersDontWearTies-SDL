#include "Audio.h"

#include "Log.h"

std::ifstream Audio::currentAudioStream = std::ifstream();
int32_t Audio::currentAudioStreamLegth = 0;

bool Audio::Initialize()
{
	if (IsInitialized()) return false;

	SDL_AudioSpec desiredAudioSpec, obtainedAudioSpec;
	SDL_memset(&desiredAudioSpec, 0, sizeof(desiredAudioSpec));
	desiredAudioSpec.freq = WAV_FREQUENCY;
	desiredAudioSpec.format = WAV_FORMAT;
	desiredAudioSpec.channels = WAV_CHANNELS;
	desiredAudioSpec.samples = WAV_SAMPLES;
	desiredAudioSpec.callback = AudioCallback;

	if (SDL_OpenAudio(&desiredAudioSpec, &obtainedAudioSpec) < 0)
	{
		Log::Print(LogTypes::Error, "Can't open audio device: %s", SDL_GetError());
		return false;
	}
	else
	{
		Log::Print(LogTypes::Info, "Audio Initialized: frequency %i, channels %u, samples %u, buffer size %u.", obtainedAudioSpec.freq, obtainedAudioSpec.channels, obtainedAudioSpec.samples, obtainedAudioSpec.size);
	}

	SDL_PauseAudio(0);

	// Workaround for the game sometimes not initializing the audio
	// quick enough, that causes the intro music to not play on a real Wii.

	SDL_Delay(50);

	return true;
}

void Audio::Dispose()
{
	StopAudio();

	if (SDL_GetAudioStatus() != SDL_AUDIO_STOPPED)
	{
		SDL_CloseAudio();
	}
}

bool Audio::LoadAudioFromWAV(const std::string baseDataPath, const std::string fileName)
{
	if (!IsInitialized()) return false;

	StopAudio();

	currentAudioStream = std::ifstream(baseDataPath + fileName, std::ios::binary);

	if (!currentAudioStream.is_open())
	{
		Log::Print(LogTypes::Error, "Can't load audio file: %s", SDL_GetError());
		return false;
	}

	currentAudioStream.seekg(0, std::ios_base::end);
	currentAudioStreamLegth = static_cast<int32_t>(currentAudioStream.tellg());

	currentAudioStream.seekg(WAV_DATA_START_POSITION, std::ios_base::beg);

	Log::Print(LogTypes::Info, "Playing audio %s...", fileName.c_str());

	return true;
}

void Audio::StopAudio()
{
	if (!currentAudioStream.is_open()) return;

	currentAudioStream.close();
	currentAudioStream = std::ifstream();
	currentAudioStreamLegth = 0;
}

void Audio::SetAudioPlaybackTime(const double elapsedTime)
{
	if (!IsInitialized()) return;
	if (!currentAudioStream.is_open()) return;

	int32_t samplePosition = static_cast<int32_t>(elapsedTime * WAV_FREQUENCY * WAV_FORMAT_BYTES * WAV_CHANNELS);

	// Make sure samplePosition is even, not odd, as audio is 16bit.
	samplePosition &= ~1;

	currentAudioStream.seekg(WAV_DATA_START_POSITION + samplePosition, std::ios_base::beg);
}

void Audio::AudioCallback(void* userdata, uint8_t* stream, int32_t len)
{
	if (currentAudioStream.is_open())
	{
		currentAudioStream.read((char*)stream, len);

		int32_t bytesRead = static_cast<int32_t>(currentAudioStream.gcount());
		int32_t remainingBytes = len - bytesRead;

		if (remainingBytes > 0)
		{
			SDL_memset(stream + bytesRead, 0, remainingBytes);

			// We have finished reading the audio file,
			// don't need to keep it open.

			currentAudioStream.close();
			currentAudioStream = std::ifstream();
			currentAudioStreamLegth = 0;
		}

		// Swap each pair of bytes due to the Wii being Big Endian

		for (int i = 0; i < bytesRead; i += 2)
		{
			uint8_t temp = stream[i + 0];
			stream[i + 0] = stream[i + 1];
			stream[i + 1] = temp;
		}
	}
	else
	{
		SDL_memset(stream, 0, len);
	}
}
