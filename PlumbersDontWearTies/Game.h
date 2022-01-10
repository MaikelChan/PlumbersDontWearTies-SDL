#pragma once

#include <fstream>
#include <string>

#include <SDL.h>

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
	std::string baseDataPath = std::string();

	_gameBinFile* gameData = nullptr;

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
	Game(std::string baseDataPath);
	~Game();

	void Start();
	void Stop();
	void Update(const double deltaSeconds);
	void Render();
	void SelectDecision(const int8_t decisionIndex);
	void SelectNextDecision();
	void SelectPreviousDecision();
	void AdvancePicture();

	inline bool IsRunning() { return currentGameState != GameStates::Stopped; }
	inline bool IsInitialized() { return gameData != nullptr; }

private:
	void SetNextScene(const _actionDef* action);
	int16_t GetSceneIndexFromID(const int16_t id);
	bool LoadAudioFromWAV(std::string fileName);
	void ToUpperCase(std::string* text);

	static void AudioCallback(void* userdata, uint8_t* stream, int32_t len);
};
