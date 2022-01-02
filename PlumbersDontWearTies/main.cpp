#include "main.h"

#include <iostream>

#include <3ds.h>

int main(int argc, char **args)
{
	// Initialize SDL

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		printf("Error initializing SDL: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	atexit(SDL_Quit);

	// Initialize the game

	Game* game = new Game();

	// Initialize game controller

	SDL_ShowCursor(SDL_DISABLE);
	hidInit();

	// Start game

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
		else if (buttonsDown & KEY_LEFT)
			game->SelectDecision(0);
		else if (buttonsDown & KEY_UP)
			game->SelectDecision(1);
		else if (buttonsDown & KEY_RIGHT)
			game->SelectDecision(2);
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

	SDL_Quit();

	return 0;
}
