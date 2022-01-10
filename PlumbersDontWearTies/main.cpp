#include "main.h"

#include "Audio.h"
#include "Game.h"
#include "Log.h"
#include "Renderer.h"

#include <iostream>

#include <3ds.h>

#if _ROMFS
	constexpr const char* BASE_DATA_PATH = "romfs:/";
#else
	constexpr const char* BASE_DATA_PATH = "sdmc:/3ds/PlumbersDontWearTies/";
#endif

int main(int argc, char **args)
{
	// Initialize SDL

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		Log::Print(LogTypes::Critical, "Error initializing SDL: %s", SDL_GetError());
		return EXIT_FAILURE;
	}

	atexit(SDL_Quit);

	// Initialize renderer

	if (!Renderer::Initialize(std::string(BASE_DATA_PATH) + "Font.ttf"))
	{
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Initialize audio

	if (!Audio::Initialize())
	{
		Renderer::Dispose();
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Initialize game controller

	SDL_ShowCursor(SDL_DISABLE);
	hidInit();

	// Initialize the game

#if _ROMFS
	romfsInit();
#endif

	Game *game = new Game(BASE_DATA_PATH);
	game->Start();

	Uint32 previousTime = SDL_GetTicks();

	while (game->IsRunning())
	{
		// Input needs to be read before polling SDL events
		hidScanInput();
		u32 buttonsDown = hidKeysDown();
		
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
			{
				game->Stop();
				break;
			}
			}
		}

		if (buttonsDown & KEY_SELECT)
			game->Stop();
		else if (buttonsDown & KEY_DOWN)
			game->SelectNextDecision();
		else if (buttonsDown & KEY_UP)
			game->SelectPreviousDecision();
		else if (buttonsDown & KEY_A)
			game->AdvancePicture();

		Uint32 currentTime = SDL_GetTicks();
		double deltaSeconds = (currentTime - previousTime) / 1000.0;
		previousTime = currentTime;

		game->Update(deltaSeconds);
		game->Render();
	}

	delete game;
	game = nullptr;

#if _ROMFS
	romfsExit();
#endif

	Audio::Dispose();
	Renderer::Dispose();
	SDL_Quit();

	return EXIT_SUCCESS;
}
