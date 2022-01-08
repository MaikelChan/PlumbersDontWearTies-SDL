#include "main.h"

#include <iostream>

#include <switch.h>

int main(int argc, char** args)
{
	// Initialize SDL

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
	{
		SDL_LogCritical(0, "Error initializing SDL: %s", SDL_GetError());
		return 1;
	}

	// Create window

	SDL_Window* window = SDL_CreateWindow("Plumbers Don't Wear Ties", 0, 0, 1920, 1080, 0);

	if (window == nullptr)
	{
		SDL_LogCritical(0, "Could not create a window: %s", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	// Initialize renderer

	//SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
	SDL_Renderer* renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == nullptr)
	{
		SDL_LogCritical(0, "Could not create a renderer: %s", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	// Initialize game controller

	SDL_Joystick *joystick = SDL_JoystickOpen(0);
	if (joystick == nullptr)
	{
		SDL_LogCritical(0, "Could not open joystick: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	// Initialize the game

	Game* game = new Game(renderer);
	game->Start();

	Uint64 previousTime = SDL_GetPerformanceCounter();

	while (game->IsRunning() && appletMainLoop())
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
				case SDL_JOYBUTTONDOWN:
				{
					switch (event.jbutton.button)
					{
						case JOY_MINUS:
							game->Stop();
							break;
						case JOY_LEFT:
							game->SelectDecision(0);
							break;
						case JOY_UP:
							game->SelectDecision(1);
							break;
						case JOY_RIGHT:
							game->SelectDecision(2);
							break;
						case JOY_A:
							game->AdvancePicture();
							break;
					}

					break;
				}
			}
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		Uint64 currentTime = SDL_GetPerformanceCounter();
		double deltaSeconds = (currentTime - previousTime) / (double)SDL_GetPerformanceFrequency();
		previousTime = currentTime;

		game->Update(deltaSeconds);

		SDL_RenderPresent(renderer);
	}

	delete game;
	game = nullptr;

	SDL_JoystickClose(joystick);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
