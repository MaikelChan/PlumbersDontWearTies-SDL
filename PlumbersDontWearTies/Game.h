#pragma once

#include <string>

#include "GameData.h"

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

	GameStates currentGameState = GameStates::Stopped;
	int16_t currentSceneIndex = 0;
	int16_t lastDecisionSceneIndex = 0;
	int16_t currentPictureIndex = 0;
	int8_t currentDecisionIndex = -1;
	int32_t currentScore = 0;
	double currentWaitTimer = 0.0;

public:
	Game(const std::string baseDataPath);
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
	void ToUpperCase(std::string* text);
};
