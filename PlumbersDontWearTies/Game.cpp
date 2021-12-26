#include "Game.h"

std::ifstream Game::currentAudioStream = std::ifstream();
int32_t Game::currentAudioStreamLegth = 0;

Game::Game(SDL_Surface *screenSurface)
{
	// Initialize class variables

#if _ROMFS
	romfsInit();
	baseDataPath = "romfs:/";
#else
	baseDataPath = "sdmc:/3ds/PlumbersDontWearTies/";
#endif

	pathSeparator = "/";

	gameData = nullptr;

	Game::screenSurface = screenSurface;
	rendererWidth = 640;
	rendererHeight = 480;

	currentTexture = nullptr;

	textFont = nullptr;
	currentTextTexture = nullptr;

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
		printf("TTF has not been initialized: %s\n", TTF_GetError());
	}

	std::string fontPath = baseDataPath + "Font.ttf";
	textFont = TTF_OpenFont(fontPath.c_str(), 24);

	if (textFont == nullptr)
	{
		printf("%s has not been found or couldn't be opened: %s\n", fontPath.c_str(), TTF_GetError());
	}

	// Load GAME.BIN

	std::ifstream gameBinStream(baseDataPath + "GAME.BIN", std::ios::binary);

	if (!gameBinStream.is_open())
	{
		printf("GAME.BIN has not been found.\n");
		return;
	}

	gameData = new _gameBinFile;
	gameBinStream.read((char *)gameData, sizeof(_gameBinFile));
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

#if _ROMFS
	romfsExit();
#endif
}

void Game::Start()
{
	if (!IsInitialized())
		return;

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

	if (SDL_OpenAudio(&desiredAudioSpec, &obtainedAudioSpec) < 0)
	{
		printf("Can't open audio device: %s\n", SDL_GetError());
		return;
	}
	else
	{
		printf("Audio Initialized: frequency %i, channels %u, samples %u, buffer size %lu.\n", obtainedAudioSpec.freq, obtainedAudioSpec.channels, obtainedAudioSpec.samples, obtainedAudioSpec.size);
	}

	SDL_PauseAudio(0);
}

void Game::Stop()
{
	if (!IsInitialized())
		return;

	currentGameState = GameStates::Stopped;

	if (currentTexture != nullptr)
	{
		SDL_FreeSurface(currentTexture);
		currentTexture = nullptr;
	}

	if (currentTextTexture != nullptr)
	{
		SDL_FreeSurface(currentTextTexture);
		currentTextTexture = nullptr;
	}

	if (SDL_GetAudioStatus() != SDL_AUDIO_STOPPED)
	{
		SDL_CloseAudio();
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
	if (!IsInitialized())
		return;

	// Update game

	_sceneDef *scene = &gameData->scenes[currentSceneIndex];

	switch (currentGameState)
	{
	case GameStates::Stopped:
	{
		return;
	}
	case GameStates::BeginScene:
	{
		printf("Entered scene %s.\n", scene->szSceneFolder);

		std::string wavPath = scene->szSceneFolder + pathSeparator + scene->szDialogWav;
		if (LoadAudioFromWAV(wavPath))
			printf("Playing %s audio file...\n", scene->szDialogWav);

		currentPictureIndex = 0;
		currentGameState = GameStates::BeginPicture;
		break;
	}
	case GameStates::BeginPicture:
	{
		_pictureDef *picture = &gameData->pictures[scene->pictureIndex + currentPictureIndex];
		std::string bmpPath = scene->szSceneFolder + pathSeparator + picture->szBitmapFile;
		LoadTextureFromBMP(bmpPath);

		currentWaitTimer = picture->duration / 10.0;
		printf("Waiting %f seconds...\n", currentWaitTimer);

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

		printf("%i decisions, waiting for input...\n", scene->numActions);

		currentDecisionIndex = -1;
		currentGameState = GameStates::WaitingDecision;

		break;
	}
	case GameStates::WaitingDecision:
	{
		if (currentDecisionIndex >= 0 && currentDecisionIndex < scene->numActions)
		{
			printf("Selected decision: %i\n", currentDecisionIndex + 1);
			PrintText(std::string());

			currentScore += scene->actions[currentDecisionIndex].scoreDelta;
			SetNextScene(&scene->actions[currentDecisionIndex]);
		}

		break;
	}
	}

	// Render picture

	if (currentTexture == nullptr)
	{
		return;
	}

	SDL_Rect textureRect;

	float rendererAspectRatio = static_cast<float>(rendererWidth) / rendererHeight;
	float textureAspectRatio = static_cast<float>(currentTexture->w) / currentTexture->h;

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

	SDL_BlitSurface(currentTexture, NULL, screenSurface, &textureRect);

	// Render text

	if (currentTextTexture != nullptr)
	{
		float scale;

		if (rendererAspectRatio > textureAspectRatio)
			scale = static_cast<float>(rendererHeight) / currentTexture->h;
		else
			scale = static_cast<float>(rendererWidth) / currentTexture->w;

		scale *= 0.5;

		SDL_Rect textRect;
		textRect.x = textureRect.x + static_cast<int32_t>(32 * scale);
		textRect.y = textureRect.y + textureRect.h - static_cast<int32_t>(80 * scale);
		textRect.w = static_cast<int32_t>(currentTextTexture->w * scale);
		textRect.h = static_cast<int32_t>(currentTextTexture->h * scale);

		SDL_BlitSurface(currentTextTexture, NULL, screenSurface, &textRect);
	}
}

void Game::SelectDecision(const int8_t decision)
{
	if (!IsInitialized())
		return;
	currentDecisionIndex = decision;
}

void Game::AdvancePicture()
{
	if (!IsInitialized())
		return;
	if (currentGameState != GameStates::WaitingPicture)
		return;

	if (currentAudioStream.is_open())
	{
		// Calculate elapsed time since beginning of scene

		int16_t startPictureIndex = gameData->scenes[currentSceneIndex].pictureIndex;
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

void Game::SetNextScene(const _actionDef *action)
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

	if (gameData->scenes[currentSceneIndex].numActions > 1)
		lastDecisionSceneIndex = currentSceneIndex;
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
		SDL_FreeSurface(currentTexture);
		currentTexture = nullptr;
	}

	ToUpperCase(&fileName);

	printf("Loading %s...", fileName.c_str());
	SDL_Surface *newTexture = SDL_LoadBMP((baseDataPath + fileName).c_str());

	if (newTexture == nullptr)
	{
		printf(" ERROR\n");
		printf("Can't load bitmap into surface: %s\n", SDL_GetError());
		return false;
	}

	printf(" OK\n");

	currentTexture = newTexture;
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
		printf("Audio has been interrupted.\n");
		return true;
	}

	ToUpperCase(&fileName);
	currentAudioStream = std::ifstream(baseDataPath + fileName, std::ios::binary);

	if (!currentAudioStream.is_open())
	{
		printf("Can't load audio file: %s\n", SDL_GetError());
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
		SDL_FreeSurface(currentTextTexture);
		currentTextTexture = nullptr;
	}

	if (text.empty())
	{
		printf("Text has been cleared.\n");
		return true;
	}

	const char *cText = text.c_str();

	if (textFont == nullptr)
	{
		printf("%s\n", cText);
		return false;
	}

	SDL_Color white = {255, 255, 255, 255};
	SDL_Surface *textSurface = TTF_RenderText_Blended(textFont, cText, white);

	if (textSurface == nullptr)
	{
		printf("Can't create text surface: %s\n", SDL_GetError());
		return false;
	}

	int w, h;
	if (TTF_SizeText(textFont, cText, &w, &h) < 0)
	{
		SDL_FreeSurface(textSurface);
		printf("Can't calculate size of text texture: %s\n", TTF_GetError());
		return false;
	}

	currentTextTexture = textSurface;

	return true;
}

void Game::ToUpperCase(std::string *text)
{
	for (auto &c : *text)
		c = toupper(c);
}

void Game::AudioCallback(void *userdata, uint8_t *stream, int len)
{
	if (currentAudioStream.is_open())
	{
		currentAudioStream.read((char *)stream, len);

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