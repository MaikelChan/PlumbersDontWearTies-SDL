#include "main.h"

#include "Audio.h"
#include "Game.h"
#include "Log.h"
#include "Renderer.h"

#include <iostream>

#include <gccore.h>
#include <wiiuse/wpad.h>

constexpr const char* BASE_DATA_PATH = "sd:/apps/PlumbersDontWearTies/Data/";

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

	PAD_Init();
	WPAD_Init();
	SDL_ShowCursor(SDL_DISABLE);

	// Initialize the game

	Game *game = new Game(BASE_DATA_PATH);
	game->Start();

	Uint32 previousTime = SDL_GetTicks();

	while (game->IsRunning())
	{
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

		PAD_ScanPads();
		WPAD_ScanPads();

		u32 gcButtonsDown = PAD_ButtonsDown(0);
		u32 wiiButtonsDown = WPAD_ButtonsDown(0);

		if ((gcButtonsDown & PAD_TRIGGER_Z) || (wiiButtonsDown & WPAD_BUTTON_MINUS))
			game->Stop();
		else if ((gcButtonsDown & PAD_BUTTON_DOWN) || (wiiButtonsDown & WPAD_BUTTON_DOWN))
			game->SelectNextDecision();
		else if ((gcButtonsDown & PAD_BUTTON_UP) || (wiiButtonsDown & WPAD_BUTTON_UP))
			game->SelectPreviousDecision();
		else if ((gcButtonsDown & PAD_BUTTON_A) || (wiiButtonsDown & WPAD_BUTTON_A))
			game->AdvancePicture();

		Uint32 currentTime = SDL_GetTicks();
		double deltaSeconds = (currentTime - previousTime) / 1000.0;
		previousTime = currentTime;

		game->Update(deltaSeconds);
		game->Render();
	}

	delete game;
	game = nullptr;

	Audio::Dispose();
	Renderer::Dispose();
	SDL_Quit();

	return EXIT_SUCCESS;
}
