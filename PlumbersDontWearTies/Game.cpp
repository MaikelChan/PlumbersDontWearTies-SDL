#include "Game.h"

std::ifstream Game::currentAudioStream = std::ifstream();
int32_t Game::currentAudioStreamLegth = 0;

Game::Game(SDL_Renderer* renderer)
{
	// Initialize class variables

	baseDataPath = "Data";
	pathSeparator = "/";
	baseDataPath = baseDataPath + pathSeparator;

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

	audioDeviceId = 0;
	currentAudioStream = std::ifstream();
	currentAudioStreamLegth = 0;

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
			return;
		}
		case GameStates::BeginScene:
		{
			SDL_Log("Entered scene %s.", scene->szSceneFolder);

			std::string wavPath = scene->szSceneFolder + pathSeparator + scene->szDialogWav;
			if (LoadAudioFromWAV(wavPath))
				SDL_Log("Playing %s audio file...", scene->szDialogWav);

			currentPictureIndex = 0;
			currentGameState = GameStates::BeginPicture;
			break;
		}
		case GameStates::BeginPicture:
		{
			_pictureDef* picture = &gameData->pictures[scene->pictureIndex + currentPictureIndex];
			std::string bmpPath = scene->szSceneFolder + pathSeparator + picture->szBitmapFile;
			LoadTextureFromBMP(bmpPath);

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

			std::string bmpPath = scene->szSceneFolder + pathSeparator + scene->szDecisionBmp;
			LoadTextureFromBMP(bmpPath);

			PrintText("Your score is: " + std::to_string(currentScore));

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

	_sceneDef* scene = &gameData->scenes[currentSceneIndex];

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

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	SDL_RenderCopy(renderer, currentTexture, NULL, &textureRect);

	// Render decision screen

	if (currentGameState == GameStates::WaitingDecision)
	{
		float scale;

		if (rendererAspectRatio > textureAspectRatio)
			scale = static_cast<float>(rendererHeight) / currentTextureHeight;
		else
			scale = static_cast<float>(rendererWidth) / currentTextureWidth;

		// Render selection rect

		if (currentDecisionIndex >= 0 && currentDecisionIndex < scene->numActions)
		{
			float totalSeconds = SDL_GetPerformanceCounter() / (float)SDL_GetPerformanceFrequency();
			uint8_t alpha = static_cast<uint8_t>((sin(totalSeconds * M_PI * 2) * 0.25 + 0.75) * 255);

			SDL_SetRenderDrawColor(renderer, alpha, alpha, alpha, 255);
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_MOD);

			SDL_Rect selectionRect;
			selectionRect.x = scene->actions[currentDecisionIndex].cHotspotTopLeft.x;
			selectionRect.y = scene->actions[currentDecisionIndex].cHotspotTopLeft.y;
			selectionRect.w = scene->actions[currentDecisionIndex].cHotspotBottomRigh.x - scene->actions[currentDecisionIndex].cHotspotTopLeft.x;
			selectionRect.h = scene->actions[currentDecisionIndex].cHotspotBottomRigh.y - scene->actions[currentDecisionIndex].cHotspotTopLeft.y;
			ScaleRect(&selectionRect, &textureRect, scale);
			SDL_RenderFillRect(renderer, &selectionRect);
		}

		// Render text

		scale *= 0.5;

		SDL_Rect textRect;
		textRect.x = 32;
		textRect.y = static_cast<int32_t>(textureRect.h / scale) - 80;
		textRect.w = currentTextTextureWidth;
		textRect.h = currentTextTextureHeight;
		ScaleRect(&textRect, &textureRect, scale);

		SDL_RenderCopy(renderer, currentTextTexture, NULL, &textRect);
	}
}

void Game::WindowSizeChanged(const int32_t width, const int32_t height)
{
	if (!IsInitialized()) return;

	rendererWidth = width;
	rendererHeight = height;

	SDL_Log("New window size: %ix%i.", width, height);
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
		PrintText(std::string());

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

bool Game::LoadTextureFromBMP(std::string fileName)
{
	if (currentTexture != nullptr)
	{
		SDL_DestroyTexture(currentTexture);
		currentTexture = nullptr;
	}

	ToUpperCase(&fileName);
	SDL_Surface* newSurface = SDL_LoadBMP((baseDataPath + fileName).c_str());

	if (newSurface == nullptr)
	{
		SDL_LogError(0, "Can't load bitmap: %s", SDL_GetError());
		return false;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_Texture* newTexture = SDL_CreateTextureFromSurface(renderer, newSurface);

	if (newTexture == nullptr)
	{
		SDL_LogError(0, "Can't create texture: %s", SDL_GetError());
		return false;
	}

	currentTextureWidth = newSurface->w;
	currentTextureHeight = newSurface->h;
	currentTexture = newTexture;

	SDL_FreeSurface(newSurface);

	SDL_Log("Loaded picture %s (%ix%i)", fileName.c_str(), currentTextureWidth, currentTextureHeight);

	return true;
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

bool Game::PrintText(const std::string text)
{
	if (currentTextTexture != nullptr)
	{
		SDL_DestroyTexture(currentTextTexture);
		currentTextTexture = nullptr;
		currentTextTextureWidth = 0;
		currentTextTextureHeight = 0;
	}

	if (text.empty())
	{
		return true;
	}

	const char* cText = text.c_str();

	if (textFont == nullptr)
	{
		SDL_Log("%s", cText);
		return false;
	}

	SDL_Color white = { 255, 255, 255, 255 };
	SDL_Surface* textSurface = TTF_RenderText_Blended(textFont, cText, white);

	if (textSurface == nullptr)
	{
		SDL_LogError(0, "Can't create text surface: %s", SDL_GetError());
		return false;
	}

	SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

	SDL_FreeSurface(textSurface);

	if (textTexture == nullptr)
	{
		SDL_LogError(0, "Can't create text texture: %s", SDL_GetError());
		return false;
	}

	int32_t w, h;
	if (TTF_SizeText(textFont, cText, &w, &h) < 0)
	{
		SDL_DestroyTexture(textTexture);
		SDL_LogError(0, "Can't calculate size of text texture: %s", TTF_GetError());
		return false;
	}

	currentTextTexture = textTexture;
	currentTextTextureWidth = w;
	currentTextTextureHeight = h;

	return true;
}

void Game::ToUpperCase(std::string* text)
{
	for (auto& c : *text)
		c = toupper(c);
}

void Game::ScaleRect(SDL_Rect* rectToScale, const SDL_Rect* textureRect, const float scale)
{
	rectToScale->x = textureRect->x + static_cast<int32_t>(rectToScale->x * scale);
	rectToScale->y = textureRect->y + static_cast<int32_t>(rectToScale->y * scale);
	rectToScale->w = static_cast<int32_t>(rectToScale->w * scale);
	rectToScale->h = static_cast<int32_t>(rectToScale->h * scale);
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