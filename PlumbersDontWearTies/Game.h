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
	std::string baseDataPath;
	std::string pathSeparator;

	_gameBinFile* gameData;

	SDL_Renderer* renderer;
	int32_t rendererWidth;
	int32_t rendererHeight;

	SDL_Texture* currentTexture;
	int32_t currentTextureWidth;
	int32_t currentTextureHeight;

	TTF_Font* textFont;
	SDL_Texture* currentTextTexture;
	int32_t currentTextTextureWidth;
	int32_t currentTextTextureHeight;

	SDL_AudioDeviceID audioDeviceId;
	static std::ifstream currentAudioStream;
	static int32_t currentAudioStreamLegth;

	GameStates currentGameState;
	int16_t currentSceneIndex;
	int16_t lastDecisionSceneIndex;
	int16_t currentPictureIndex;
	int8_t currentDecisionIndex;
	int32_t currentScore;
	double currentWaitTimer;

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
	void ScaleRect(SDL_Rect* rectToScale, const SDL_Rect* textureRect, const float scale);

	static void AudioCallback(void* userdata, uint8_t* stream, int32_t len);
};
