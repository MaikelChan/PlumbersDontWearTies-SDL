#include "Game.h"

Game::Game(SDL_Renderer* renderer)
{
	// Initialize class variables

	baseDataPath = "Data";
	baseDataPath = baseDataPath.append(PATH_SEPARATOR);
	gameData = nullptr;

	Game::renderer = renderer;
	rendererWidth = 0;
	rendererHeight = 0;

	currentTexture = nullptr;
	currentTextureWidth = 0;
	currentTextureHeight = 0;

	textFont = nullptr;
	currentTextTexture = nullptr;
	currentTextTextureWidth = 0;
	currentTextTextureHeight = 0;

	currentAudioDeviceId = 0;
	currentAudioBuffer = nullptr;

	currentGameState = GameStates::Stopped;
	currentSceneIndex = 0;
	lastDecisionSceneIndex = 0;
	currentPictureIndex = 0;
	currentDecisionIndex = -1;
	currentScore = 0;
	currentWaitTimer = 0.0;

	// Load font

	if (TTF_Init() < 0)
	{
		SDL_LogError(0, "TTF has not been initialized: %s", TTF_GetError());
	}

	std::string fontPath = baseDataPath + "Font.ttf";
	textFont = TTF_OpenFont(fontPath.c_str(), 48);

	if (textFont == nullptr)
	{
		SDL_LogError(0, "%s has not been found or couldn't be opened: %s", fontPath.c_str(), TTF_GetError());
	}

	// Load GAME.BIN

	std::string gameBinPath = baseDataPath + "GAME.BIN";
	FILE* gameDataFileHandle = fopen(gameBinPath.c_str(), "rb");
	if (gameDataFileHandle == nullptr)
	{
		SDL_LogCritical(0, "GAME.BIN has not been found.");
		return;
	}

	gameData = new _gameBinFile;
	fread(gameData, sizeof(_gameBinFile), 1, gameDataFileHandle);
	fclose(gameDataFileHandle);
}

Game::~Game()
{
	if (gameData != nullptr)
	{
		delete gameData;
		gameData = nullptr;
	}

	if (textFont != nullptr)
	{
		TTF_CloseFont(textFont);
		textFont = nullptr;
	}

	if (TTF_WasInit())
	{
		TTF_Quit();
	}
}

void Game::Start()
{
	if (!IsInitialized()) return;

	int rw, rh;
	if (SDL_GetRendererOutputSize(renderer, &rw, &rh) < 0)
	{
		SDL_LogCritical(0, "Could not get renderer output size: %s", SDL_GetError());
		return;
	}

	WindowSizeChanged(rw, rh);

	currentGameState = GameStates::BeginScene;
	currentSceneIndex = 0;
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

	if (currentTexture != nullptr)
	{
		SDL_DestroyTexture(currentTexture);
		currentTexture = nullptr;
	}

	if (currentTextTexture != nullptr)
	{
		SDL_DestroyTexture(currentTextTexture);
		currentTextTexture = nullptr;
	}

	if (currentAudioDeviceId > 0)
	{
		SDL_CloseAudioDevice(currentAudioDeviceId);
		currentAudioDeviceId = 0;
	}

	if (currentAudioBuffer != nullptr)
	{
		SDL_FreeWAV(currentAudioBuffer);
		currentAudioBuffer = nullptr;
	}
}

void Game::Update(const double deltaSeconds)
{
	if (!IsInitialized()) return;

	// Update game

	_sceneDef* scene = &gameData->scenes[currentSceneIndex];

	switch (currentGameState)
	{
		case GameStates::Stopped:
		{
			return;
		}
		case GameStates::BeginScene:
		{
			std::string wavPath = baseDataPath + scene->szSceneFolder + PATH_SEPARATOR + scene->szDialogWav;
			LoadAudioFromWAV(wavPath);

			SDL_Log("Loaded scene %s, playing %s audio file...", scene->szSceneFolder, scene->szDialogWav);

			currentPictureIndex = 0;
			currentGameState = GameStates::BeginPicture;
			break;
		}
		case GameStates::BeginPicture:
		{
			_pictureDef* picture = &gameData->pictures[scene->pictureIndex + currentPictureIndex];
			std::string bmpPath = baseDataPath + scene->szSceneFolder + PATH_SEPARATOR + picture->szBitmapFile;
			LoadTextureFromBMP(bmpPath);

			currentWaitTimer = picture->duration / 10.0;

			SDL_Log("Loaded picture %s, waiting %f seconds...", picture->szBitmapFile, currentWaitTimer);

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

			std::string bmpPath = baseDataPath + scene->szSceneFolder + PATH_SEPARATOR + scene->szDecisionBmp;
			LoadTextureFromBMP(bmpPath);

			PrintText("Your score is: %i", currentScore);

			SDL_Log("%i decisions to choose, waiting for player input...", scene->numActions);

			currentDecisionIndex = -1;
			currentGameState = GameStates::WaitingDecision;

			break;
		}
		case GameStates::WaitingDecision:
		{
			if (currentDecisionIndex >= 0 && currentDecisionIndex < scene->numActions)
			{
				SDL_Log("Selected decision: %i", currentDecisionIndex + 1);
				PrintText("");

				currentScore += scene->actions[currentDecisionIndex].scoreDelta;
				SetNextScene(&scene->actions[currentDecisionIndex]);
			}

			break;
		}
	}

	// Render picture

	SDL_Rect textureRect;

	float rendererAspectRatio = static_cast<float>(rendererWidth) / rendererHeight;
	float textureAspectRatio = static_cast<float>(currentTextureWidth) / currentTextureHeight;

	if (rendererAspectRatio > textureAspectRatio)
	{
		int32_t gameWidth = static_cast<int32_t>(rendererHeight * textureAspectRatio);
		textureRect.x = (rendererWidth - gameWidth) >> 1;
		textureRect.y = 0;
		textureRect.w = gameWidth;
		textureRect.h = rendererHeight;
	}
	else
	{
		int32_t gameHeight = static_cast<int32_t>(rendererWidth / textureAspectRatio);
		textureRect.x = 0;
		textureRect.y = (rendererHeight - gameHeight) >> 1;
		textureRect.w = rendererWidth;
		textureRect.h = gameHeight;
	}

	SDL_RenderCopy(renderer, currentTexture, NULL, &textureRect);

	// Render text

	if (currentTextTexture != nullptr)
	{
		float scale;

		if (rendererAspectRatio > textureAspectRatio)
			scale = static_cast<float>(rendererHeight) / currentTextureHeight;
		else
			scale = static_cast<float>(rendererWidth) / currentTextureWidth;

		scale *= 0.5;

		SDL_Rect textRect;
		textRect.x = textureRect.x + static_cast<int32_t>(32 * scale);
		textRect.y = textureRect.y + textureRect.h - static_cast<int32_t>(80 * scale);
		textRect.w = static_cast<int32_t>(currentTextTextureWidth * scale);
		textRect.h = static_cast<int32_t>(currentTextTextureHeight * scale);

		SDL_RenderCopy(renderer, currentTextTexture, NULL, &textRect);
	}
}

void Game::WindowSizeChanged(const int32_t width, const int32_t height)
{
	if (!IsInitialized()) return;

	rendererWidth = width;
	rendererHeight = height;

	SDL_Log("Changed window size: %ix%i", width, height);
}

void Game::SelectDecision(const int8_t decision)
{
	if (!IsInitialized()) return;
	currentDecisionIndex = decision;
}

void Game::AdvancePicture()
{
	if (!IsInitialized()) return;
	if (currentGameState != GameStates::WaitingPicture) return;

	currentWaitTimer = 0;
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
		LoadAudioFromWAV("");
	}
	else
	{
		if (action->sceneSegment == SEGMENT_DECISION)
			currentGameState = GameStates::BeginDecision;
		else
			currentGameState = GameStates::BeginScene;

		nextSceneIndex = GetSceneIndexFromID(id);
	}

	SDL_Log("Previous scene %s, index %i", gameData->scenes[currentSceneIndex].szSceneFolder, currentSceneIndex);

	if (gameData->scenes[currentSceneIndex].numActions > 1) lastDecisionSceneIndex = currentSceneIndex;
	currentSceneIndex = nextSceneIndex;
	currentPictureIndex = 0;
	currentDecisionIndex = -1;
}

int16_t Game::GetSceneIndexFromID(const int16_t id)
{
	char sceneName[5];
	sprintf(sceneName, "SC%02d", id);

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

void Game::LoadTextureFromBMP(const std::string fileName)
{
	if (currentTexture != nullptr)
	{
		SDL_DestroyTexture(currentTexture);
		currentTexture = nullptr;
	}

	SDL_Surface* bmpSurface = SDL_LoadBMP(fileName.c_str());

	if (bmpSurface == nullptr)
	{
		SDL_LogError(0, "Can't load bitmap: %s", SDL_GetError());
		return;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_Texture* newTexture = SDL_CreateTextureFromSurface(renderer, bmpSurface);

	SDL_FreeSurface(bmpSurface);

	if (newTexture == nullptr)
	{
		SDL_LogError(0, "Can't create texture: %s", SDL_GetError());
		return;
	}

	if (SDL_QueryTexture(newTexture, NULL, NULL, &currentTextureWidth, &currentTextureHeight) < 0)
	{
		SDL_DestroyTexture(newTexture);
		SDL_LogError(0, "Can't query texture: %s", SDL_GetError());
		return;
	}

	currentTexture = newTexture;
}

void Game::LoadAudioFromWAV(const std::string fileName)
{
	if (currentAudioDeviceId > 0)
	{
		SDL_CloseAudioDevice(currentAudioDeviceId);
		currentAudioDeviceId = 0;
	}

	if (currentAudioBuffer != nullptr)
	{
		SDL_FreeWAV(currentAudioBuffer);
		currentAudioBuffer = nullptr;
	}

	if (fileName.empty())
	{
		SDL_Log("Audio has been interrupted.");
		return;
	}

	SDL_AudioSpec wavSpec;
	uint8_t* wavBuffer;
	uint32_t wavLength;

	if (SDL_LoadWAV(fileName.c_str(), &wavSpec, &wavBuffer, &wavLength) == nullptr)
	{
		SDL_LogError(0, "Can't load audio file: %s", SDL_GetError());
		return;
	}

	SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, 0);

	if (deviceId == 0)
	{
		SDL_FreeWAV(wavBuffer);
		SDL_LogError(0, "Can't open audio device: %s", SDL_GetError());
		return;
	}

	if (SDL_QueueAudio(deviceId, wavBuffer, wavLength) < 0)
	{
		SDL_FreeWAV(wavBuffer);
		SDL_CloseAudioDevice(deviceId);
		SDL_LogError(0, "Can't enqueue audio: %s", SDL_GetError());
		return;
	}

	currentAudioDeviceId = deviceId;
	currentAudioBuffer = wavBuffer;

	SDL_PauseAudioDevice(deviceId, 0);
}

void Game::PrintText(const char* text, ...)
{
	if (currentTextTexture != nullptr)
	{
		SDL_DestroyTexture(currentTextTexture);
		currentTextTexture = nullptr;
		currentTextTextureWidth = 0;
		currentTextTextureHeight = 0;
	}

	if (strlen(text) == 0)
	{
		SDL_Log("Text has been cleared.");
		return;
	}

	va_list args;
	va_start(args, text);
	char finalText[256];
	vsprintf(finalText, text, args);
	va_end(args);

	SDL_Color White = { 255, 255, 255, 255 };
	SDL_Surface* textSurface = TTF_RenderText_Blended(textFont, finalText, White);

	if (textSurface == nullptr)
	{
		SDL_LogError(0, "Can't create text surface: %s", SDL_GetError());
		return;
	}

	SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

	SDL_FreeSurface(textSurface);

	if (textTexture == nullptr)
	{
		SDL_LogError(0, "Can't create text texture: %s", SDL_GetError());
		return;
	}

	int32_t w, h;
	if (TTF_SizeText(textFont, finalText, &w, &h) < 0)
	{
		SDL_DestroyTexture(textTexture);
		SDL_LogError(0, "Can't calculate size of text texture: %s", TTF_GetError());
		return;
	}

	currentTextTexture = textTexture;
	currentTextTextureWidth = w;
	currentTextTextureHeight = h;
}
