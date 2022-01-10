#include "Game.h"

#include <fstream>

#include "Audio.h"
#include "Log.h"
#include "Renderer.h"

Game::Game(const std::string baseDataPath)
{
	Game::baseDataPath = baseDataPath;

	// Load GAME.BIN

	std::ifstream gameBinStream(baseDataPath + "GAME.BIN", std::ios::binary);

	if (!gameBinStream.is_open())
	{
		Log::Print(LogTypes::Critical, "GAME.BIN has not been found.");
		return;
	}

	gameData = new _gameBinFile;
	gameBinStream.read((char*)gameData, sizeof(_gameBinFile));
	gameBinStream.close();
	gameData->SwapEndianness();
}

Game::~Game()
{
	if (gameData != nullptr)
	{
		delete gameData;
		gameData = nullptr;
	}
}

void Game::Start()
{
	if (!IsInitialized()) return;

	currentGameState = GameStates::BeginScene;
	currentSceneIndex = 1; // Skip the PC CD-Rom info screens
	lastDecisionSceneIndex = 0;
	currentPictureIndex = 0;
	currentDecisionIndex = -1;
	currentScore = 0;
	currentWaitTimer = 0.0;
}

void Game::Stop()
{
	if (!IsInitialized()) return;

	currentGameState = GameStates::Stopped;
}

void Game::Update(const double deltaSeconds)
{
	if (!IsInitialized()) return;

	_sceneDef* scene = &gameData->scenes[currentSceneIndex];

	switch (currentGameState)
	{
		case GameStates::Stopped:
		{
			break;
		}
		case GameStates::BeginScene:
		{
			Log::Print(LogTypes::Info, "Entered scene %s.", scene->szSceneFolder);

			std::string wavPath = scene->szSceneFolder + std::string("/") + scene->szDialogWav;
			ToUpperCase(&wavPath);
			Audio::LoadAudioFromWAV(baseDataPath, wavPath);

			currentPictureIndex = 0;
			currentGameState = GameStates::BeginPicture;
			break;
		}
		case GameStates::BeginPicture:
		{
			_pictureDef* picture = &gameData->pictures[scene->pictureIndex + currentPictureIndex];
			std::string bmpPath = scene->szSceneFolder + std::string("/") + picture->szBitmapFile;
			ToUpperCase(&bmpPath);
			Renderer::LoadPictureFromBMP(baseDataPath, bmpPath);

			currentWaitTimer = picture->duration / 10.0;
			Log::Print(LogTypes::Info, "Waiting %f seconds...", currentWaitTimer);

			currentGameState = GameStates::WaitingPicture;
			break;
		}
		case GameStates::WaitingPicture:
		{
			currentWaitTimer -= deltaSeconds;
			if (currentWaitTimer <= 0)
			{
				currentWaitTimer = 0;
				currentPictureIndex++;
				if (currentPictureIndex >= scene->numPics)
					currentGameState = GameStates::BeginDecision;
				else
					currentGameState = GameStates::BeginPicture;
			}

			break;
		}
		case GameStates::BeginDecision:
		{
			if (scene->numActions == 1)
			{
				SetNextScene(&scene->actions[0]);
				break;
			}

			std::string bmpPath = scene->szSceneFolder + std::string("/") + scene->szDecisionBmp;
			ToUpperCase(&bmpPath);
			Renderer::LoadPictureFromBMP(baseDataPath, bmpPath);

			Renderer::GenerateScoreText("Your score is: " + std::to_string(currentScore));

			Log::Print(LogTypes::Info, "%i decisions, waiting for input...", scene->numActions);

			currentDecisionIndex = -1;
			currentGameState = GameStates::WaitingDecision;

			break;
		}
		case GameStates::WaitingDecision:
		{
			break;
		}
		default:
		{
			Log::Print(LogTypes::Error, "State %i is not implemented.", currentGameState);
			break;
		}
	}
}

void Game::Render()
{
	if (!IsInitialized()) return;

	Renderer::Clear(0, 0, 0);
	Renderer::RenderPicture();

	if (currentGameState == GameStates::WaitingDecision)
	{
		_sceneDef* scene = &gameData->scenes[currentSceneIndex];

		if (currentDecisionIndex >= 0 && currentDecisionIndex < scene->numActions)
		{
			_actionDef* action = &scene->actions[currentDecisionIndex];
			int32_t x = action->cHotspotTopLeft.x;
			int32_t y = action->cHotspotTopLeft.y;
			int32_t w = action->cHotspotBottomRigh.x - action->cHotspotTopLeft.x;
			int32_t h = action->cHotspotBottomRigh.y - action->cHotspotTopLeft.y;

			Renderer::RenderDecisionSelection(x, y, w, h);
		}

		Renderer::RenderScore();
	}

	Renderer::Present();
}

void Game::SelectDecision(const int8_t decision)
{
	if (!IsInitialized()) return;
	if (currentGameState != GameStates::WaitingDecision) return;

	if (decision < 0) return;
	if (decision >= gameData->scenes[currentSceneIndex].numActions) return;

	currentDecisionIndex = decision;
}

void Game::SelectNextDecision()
{
	if (!IsInitialized()) return;
	if (currentGameState != GameStates::WaitingDecision) return;

	if (currentDecisionIndex < 0)
	{
		currentDecisionIndex = 0;
		return;
	}

	int16_t numActions = gameData->scenes[currentSceneIndex].numActions;
	if (currentDecisionIndex < numActions - 1) currentDecisionIndex++;
}

void Game::SelectPreviousDecision()
{
	if (!IsInitialized()) return;
	if (currentGameState != GameStates::WaitingDecision) return;

	if (currentDecisionIndex < 0)
	{
		currentDecisionIndex = gameData->scenes[currentSceneIndex].numActions - 1;
		return;
	}

	if (currentDecisionIndex > 0) currentDecisionIndex--;
}

void Game::AdvancePicture()
{
	if (!IsInitialized()) return;

	_sceneDef* scene = &gameData->scenes[currentSceneIndex];

	if (currentGameState == GameStates::WaitingPicture)
	{
		// Calculate elapsed time since beginning of scene

		int16_t startPictureIndex = scene->pictureIndex;
		int16_t endPictureIndex = startPictureIndex + currentPictureIndex + 1;

		double elapsedTime = 0.0;
		for (int16_t t = startPictureIndex; t < endPictureIndex; t++)
		{
			elapsedTime += gameData->pictures[t].duration / 10.0;
		}

		Audio::SetAudioPlaybackTime(elapsedTime);

		currentWaitTimer = 0;
	}
	else if (currentGameState == GameStates::WaitingDecision)
	{
		if (currentDecisionIndex < 0) return;
		if (currentDecisionIndex >= scene->numActions) return;

		Log::Print(LogTypes::Info, "Selected decision: %i", currentDecisionIndex + 1);
		Renderer::GenerateScoreText(std::string());

		currentScore += scene->actions[currentDecisionIndex].scoreDelta;
		SetNextScene(&scene->actions[currentDecisionIndex]);
	}
}

void Game::SetNextScene(const _actionDef* action)
{
	int16_t id = action->nextSceneID;

	if (id == SCENEID_ENDGAME)
	{
		Stop();
		return;
	}

	int16_t nextSceneIndex;

	if (id == SCENEID_PREVDECISION)
	{
		currentGameState = GameStates::BeginDecision;
		nextSceneIndex = lastDecisionSceneIndex;

		// Going to previous decision implies changing the scene,
		// so interrupt the audio of current scene in case it's still playing.
		Audio::StopAudio();
	}
	else
	{
		if (action->sceneSegment == SEGMENT_DECISION)
			currentGameState = GameStates::BeginDecision;
		else
			currentGameState = GameStates::BeginScene;

		nextSceneIndex = GetSceneIndexFromID(id);
	}

	if (gameData->scenes[currentSceneIndex].numActions > 1) lastDecisionSceneIndex = currentSceneIndex;
	currentSceneIndex = nextSceneIndex;
	currentPictureIndex = 0;
	currentDecisionIndex = -1;
}

int16_t Game::GetSceneIndexFromID(const int16_t id)
{
	char sceneName[10];
#ifdef _MSC_VER
	sprintf_s(sceneName, sizeof(sceneName), "SC%02d", id);
#else
	sprintf(sceneName, "SC%02d", id);
#endif

	for (int16_t s = 0; s < gameData->numScenes; s++)
	{
		int32_t result = strcmp(gameData->scenes[s].szSceneFolder, sceneName);
		if (result == 0)
		{
			return s;
		}
	}

	return 0;
}

void Game::ToUpperCase(std::string* text)
{
	for (auto& c : *text)
		c = toupper(c);
}
