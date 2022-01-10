#pragma once

#include <fstream>
#include <string>

#include <SDL.h>
#include <SDL_ttf.h>

#include "GameData.h"

// Format of game's WAV files

#define WAV_FREQUENCY 11025 // Hz
#define WAV_FORMAT 2 // 2 bytes, 16 bits
#define WAV_CHANNELS 2 // Stereo
#define WAV_SAMPLES 256

// TODO: Does audio data in a WAV always start in the same offset?

#define WAV_DATA_START_POSITION 0x2c

enum class GameStates
{
	Stopped,
	BeginScene,
	BeginPicture,
	WaitingPicture,
	BeginDecision,
	WaitingDecision
};

class Game
{
private:
	std::string baseDataPath = "Data/";
	std::string pathSeparator = "/";

	_gameBinFile* gameData = nullptr;

	SDL_Renderer* renderer = nullptr;
	int32_t rendererWidth = 0;
	int32_t rendererHeight = 0;
	SDL_Rect viewportRect = {};
	float viewportScale = 0;

	SDL_Texture* currentTexture = nullptr;
	int32_t currentTextureWidth = 0;
	int32_t currentTextureHeight = 0;

	TTF_Font* textFont = nullptr;
	SDL_Texture* currentTextTexture = nullptr;
	int32_t currentTextTextureWidth = 0;
	int32_t currentTextTextureHeight = 0;

	SDL_AudioDeviceID audioDeviceId = 0;
	static std::ifstream currentAudioStream;
	static int32_t currentAudioStreamLegth;

	GameStates currentGameState = GameStates::Stopped;
	int16_t currentSceneIndex = 0;
	int16_t lastDecisionSceneIndex = 0;
	int16_t currentPictureIndex = 0;
	int8_t currentDecisionIndex = -1;
	int32_t currentScore = 0;
	double currentWaitTimer = 0.0;

public:
	Game(SDL_Renderer* renderer);
	~Game();

	void Start();
	void Stop();
	void Update(const double deltaSeconds);
	void Render();
	void WindowSizeChanged(const int32_t width, const int32_t height);
	void SelectDecision(const int8_t decisionIndex);
	void SelectNextDecision();
	void SelectPreviousDecision();
	void AdvancePicture();

	inline bool IsRunning() { return currentGameState != GameStates::Stopped; }
	inline bool IsInitialized() { return gameData != nullptr; }

private:
	void SetNextScene(const _actionDef* action);
	int16_t GetSceneIndexFromID(const int16_t id);
	bool LoadTextureFromBMP(std::string fileName);
	bool LoadAudioFromWAV(std::string fileName);
	bool PrintText(const std::string text);
	void ToUpperCase(std::string* text);
	void ScaleRect(SDL_Rect* rectToScale, const float scale);
	void UpdateViewport();

	static void AudioCallback(void* userdata, uint8_t* stream, int32_t len);
};
