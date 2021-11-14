#pragma once

#include <string>
#include <cstring>

#include <SDL.h>
#include <SDL_ttf.h>

#include "GameData.h"

#define PATH_SEPARATOR "/"

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

	SDL_AudioDeviceID currentAudioDeviceId;
	uint8_t* currentAudioBuffer;

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
	void WindowSizeChanged(const int32_t width, const int32_t height);
	void SelectDecision(const int8_t decisionIndex);
	void AdvancePicture();

	inline bool IsRunning() { return currentGameState != GameStates::Stopped; }
	inline bool IsInitialized() { return gameData != nullptr; }

private:
	void SetNextScene(const _actionDef* action);
	int16_t GetSceneIndexFromID(const int16_t id);
	void LoadTextureFromBMP(const std::string fileName);
	void LoadAudioFromWAV(const std::string fileName);
	void PrintText(const char* text, ...);
};
