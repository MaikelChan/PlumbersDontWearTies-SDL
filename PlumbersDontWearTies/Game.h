#pragma once

#include <fstream>
#include <string>

#include <3ds.h>

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#include "GameData.h"

// Format of game's WAV files

#define WAV_FREQUENCY 11025 // Hz
#define WAV_FORMAT 2 // 2 bytes, 16 bits
#define WAV_CHANNELS 2 // Stereo
#define WAV_SAMPLES 1024 // SDL Wii always ignores this and sets it to 1152

// TODO: Does audio data in a WAV always start in the same offset?

#define WAV_DATA_START_POSITION 0x2c

// Thread related stuff

#define STACK_SIZE (4 * 1024)

enum class GameStates
{
	Stopped,
	BeginScene,
	BeginPicture,
	LoadingPicture,
	WaitingPicture,
	BeginDecision,
	LoadingDecisionPicture,
	WaitingDecision
};

struct PictureLoadingInfo
{
	enum class States
	{
		None,
		Loading,
		LoadingSuccess,
		LoadingFailure
	};

	States state;
	std::string pictureName;
	SDL_Surface *pictureSurface;

	void Reset()
	{
		state = States::None;
		pictureName = std::string();
		pictureSurface = nullptr;
	}
};

class Game
{
private:
	std::string baseDataPath;
	std::string pathSeparator;

	_gameBinFile* gameData;

	SDL_Surface* screenSurface;
	int32_t rendererWidth;
	int32_t rendererHeight;

	SDL_Surface* currentTexture;

	TTF_Font* textFont;
	SDL_Surface* currentTextTexture;

	static std::ifstream currentAudioStream;
	static int32_t currentAudioStreamLegth;

	GameStates currentGameState;
	int16_t currentSceneIndex;
	int16_t lastDecisionSceneIndex;
	int16_t currentPictureIndex;
	int8_t currentDecisionIndex;
	int32_t currentScore;
	double currentWaitTimer;

	Thread loadingThread;
	s32 mainThreadPriority;
	PictureLoadingInfo pictureLoadingInfo;

public:
	Game(SDL_Surface* screenSurface);
	~Game();

	void Start();
	void Stop();
	void Update(const double deltaSeconds);
	void SelectDecision(const int8_t decisionIndex);
	void AdvancePicture();

	inline bool IsRunning() { return currentGameState != GameStates::Stopped; }
	inline bool IsInitialized() { return gameData != nullptr; }

private:
	void SetNextScene(const _actionDef* action);
	int16_t GetSceneIndexFromID(const int16_t id);
	void LoadTextureFromBMP(std::string fileName);
	bool LoadAudioFromWAV(std::string fileName);
	bool PrintText(const std::string text);
	void ToUpperCase(std::string* text);

	static void AudioCallback(void* userdata, uint8_t* stream, int len);
};
