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

	// Initialize video

	SDL_Surface *screenSurface = SDL_SetVideoMode(640, 480, 16, SDL_DOUBLEBUF | SDL_FITHEIGHT);
	if (screenSurface == nullptr)
	{
		printf("Unable to set video: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	// Initialize game controller

	//SDL_ShowCursor(SDL_DISABLE);

	// Initialize the game

	Game *game = new Game(screenSurface);
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

		hidScanInput();
		u32 buttonsDown = hidKeysDown();

		if (buttonsDown != 0)
		{
			printf("Buttons: %lu", buttonsDown);
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

		SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 64, 16, 32));

		Uint32 currentTime = SDL_GetTicks();
		double deltaSeconds = (currentTime - previousTime) / 1000.0;
		previousTime = currentTime;

		game->Update(deltaSeconds);

		SDL_Flip(screenSurface);
	}

	delete game;
	game = nullptr;

	SDL_FreeSurface(screenSurface);
	SDL_Quit();

	return 0;
}
