#include "Game.h"
#include "Renderer.h"

std::ifstream Game::currentAudioStream = std::ifstream();
int32_t Game::currentAudioStreamLegth = 0;

Game::Game(std::string baseDataPath)
{
	Game::baseDataPath = baseDataPath;

	// Load GAME.BIN

	std::ifstream gameBinStream(baseDataPath + "GAME.BIN", std::ios::binary);

	if (!gameBinStream.is_open())
	{
		SDL_LogCritical(0, "GAME.BIN has not been found.");
		return;
	}

	gameData = new _gameBinFile;
	gameBinStream.read((char*)gameData, sizeof(_gameBinFile));
	gameBinStream.close();
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

	// Initialize audio

	SDL_AudioSpec desiredAudioSpec, obtainedAudioSpec;
	SDL_memset(&desiredAudioSpec, 0, sizeof(desiredAudioSpec));
	desiredAudioSpec.freq = WAV_FREQUENCY;
	desiredAudioSpec.format = AUDIO_S16;
	desiredAudioSpec.channels = WAV_CHANNELS;
	desiredAudioSpec.samples = WAV_SAMPLES;
	desiredAudioSpec.callback = AudioCallback;
	audioDeviceId = SDL_OpenAudioDevice(NULL, 0, &desiredAudioSpec, &obtainedAudioSpec, /*SDL_AUDIO_ALLOW_FORMAT_CHANGE*/ 0);

	if (audioDeviceId == 0)
	{
		SDL_LogError(0, "Can't open audio device: %s", SDL_GetError());
		return;
	}
	else
	{
		SDL_Log("Audio Initialized: frequency %i, channels %u, samples %u, buffer size %u.", obtainedAudioSpec.freq, obtainedAudioSpec.channels, obtainedAudioSpec.samples, obtainedAudioSpec.size);
	}

	SDL_PauseAudioDevice(audioDeviceId, 0);
}

void Game::Stop()
{
	if (!IsInitialized()) return;

	currentGameState = GameStates::Stopped;

	if (audioDeviceId > 0)
	{
		SDL_CloseAudioDevice(audioDeviceId);
		audioDeviceId = 0;
	}

	if (currentAudioStream.is_open())
	{
		currentAudioStream.close();
		currentAudioStream = std::ifstream();
		currentAudioStreamLegth = 0;
	}
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
			SDL_Log("Entered scene %s.", scene->szSceneFolder);

			std::string wavPath = scene->szSceneFolder + std::string("/") + scene->szDialogWav;
			if (LoadAudioFromWAV(wavPath))
				SDL_Log("Playing %s audio file...", scene->szDialogWav);

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
			SDL_Log("Waiting %f seconds...", currentWaitTimer);

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

			SDL_Log("%i decisions to choose, waiting for player input...", scene->numActions);

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
			SDL_LogError(0, "State %i is not implemented.", currentGameState);
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
			float totalSeconds = SDL_GetPerformanceCounter() / (float)SDL_GetPerformanceFrequency();
			uint8_t alpha = static_cast<uint8_t>((sin(totalSeconds * M_PI * 2) * 0.25 + 0.75) * 255);

			int32_t x = scene->actions[currentDecisionIndex].cHotspotTopLeft.x;
			int32_t y = scene->actions[currentDecisionIndex].cHotspotTopLeft.y;
			int32_t w = scene->actions[currentDecisionIndex].cHotspotBottomRigh.x - scene->actions[currentDecisionIndex].cHotspotTopLeft.x;
			int32_t h = scene->actions[currentDecisionIndex].cHotspotBottomRigh.y - scene->actions[currentDecisionIndex].cHotspotTopLeft.y;

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
		if (currentAudioStream.is_open())
		{
			// Calculate elapsed time since beginning of scene

			int16_t startPictureIndex = scene->pictureIndex;
			int16_t endPictureIndex = startPictureIndex + currentPictureIndex + 1;

			double elapsedTime = 0.0;
			for (int16_t t = startPictureIndex; t < endPictureIndex; t++)
			{
				elapsedTime += gameData->pictures[t].duration / 10.0;
			}

			int32_t samplePosition = static_cast<int32_t>(elapsedTime * WAV_FREQUENCY * WAV_FORMAT * WAV_CHANNELS);

			// Make sure samplePosition is even, not odd, as audio is 16bit.

			samplePosition &= ~1;

			currentAudioStream.seekg(WAV_DATA_START_POSITION + samplePosition, std::ios_base::beg);
		}

		currentWaitTimer = 0;
	}
	else if (currentGameState == GameStates::WaitingDecision)
	{
		if (currentDecisionIndex < 0) return;
		if (currentDecisionIndex >= scene->numActions) return;

		SDL_Log("Selected decision: %i", currentDecisionIndex + 1);
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
		LoadAudioFromWAV(std::string());
	}
	else
	{
		if (action->sceneSegment == SEGMENT_DECISION)
			currentGameState = GameStates::BeginDecision;
		else
			currentGameState = GameStates::BeginScene;

		nextSceneIndex = GetSceneIndexFromID(id);
	}

	//SDL_Log("Previous scene %s, index %i", gameData->scenes[currentSceneIndex].szSceneFolder, currentSceneIndex);

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

bool Game::LoadAudioFromWAV(std::string fileName)
{
	if (currentAudioStream.is_open())
	{
		currentAudioStream.close();
		currentAudioStream = std::ifstream();
		currentAudioStreamLegth = 0;
	}

	if (fileName.empty())
	{
		SDL_Log("Audio has been interrupted.");
		return true;
	}

	ToUpperCase(&fileName);
	currentAudioStream = std::ifstream(baseDataPath + fileName, std::ios::binary);

	if (!currentAudioStream.is_open())
	{
		SDL_LogError(0, "Can't load audio file: %s", SDL_GetError());
		return false;
	}

	currentAudioStream.seekg(0, std::ios_base::end);
	currentAudioStreamLegth = static_cast<int32_t>(currentAudioStream.tellg());

	// TODO: Does audio data in a WAV always start in the same offset?
	currentAudioStream.seekg(WAV_DATA_START_POSITION, std::ios_base::beg);

	return true;
}

void Game::ToUpperCase(std::string* text)
{
	for (auto& c : *text)
		c = toupper(c);
}

void Game::AudioCallback(void* userdata, uint8_t* stream, int32_t len)
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
	}
	else
	{
		SDL_memset(stream, 0, len);
	}
}