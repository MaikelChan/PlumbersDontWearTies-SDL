#include "main.h"

#include "Audio.h"
#include "Game.h"
#include "Log.h"
#include "Renderer.h"

#include <iostream>

#include <whb/proc.h>
#include <whb/sdcard.h>

int main(int argc, char** args)
{
	// Initialize SDL

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
	{
		Log::Print(LogTypes::Critical, "Error initializing SDL: %s", SDL_GetError());
		return EXIT_FAILURE;
	}

	// Create window

	SDL_Window* window = SDL_CreateWindow("Plumbers Don't Wear Ties", 0, 0, 1280, 720, 0);

	if (window == nullptr)
	{
		Log::Print(LogTypes::Critical, "Could not create a window: %s", SDL_GetError());
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Initialize SD card

	if (!WHBMountSdCard())
	{
		Log::Print(LogTypes::Critical, "Could not mount SD card.");
		SDL_Quit();
		return EXIT_FAILURE;
	}

	char *sdRootPath = WHBGetSdCardMountPath();
	Log::Print(LogTypes::Info, "SD root path: %s", sdRootPath);
	char path[256];
	sprintf(path, "%s/PlumbersDontWearTies/", sdRootPath);

	// Initialize renderer

	if (!Renderer::Initialize(window, std::string(path) + "Font.ttf"))
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

	SDL_Joystick *joystick = SDL_JoystickOpen(0);
	if (joystick == nullptr)
	{
		SDL_LogCritical(0, "Could not open joystick: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	// Initialize the game

	Game* game = new Game(std::string(path));
	game->Start();

	Uint64 previousTime = SDL_GetPerformanceCounter();

	while (game->IsRunning() && WHBProcIsRunning())
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
						case JOY_DOWN:
						case DPAD_DOWN:
							game->SelectNextDecision();
							break;
						case JOY_UP:
						case DPAD_UP:
							game->SelectPreviousDecision();
							break;
						case JOY_A:
							game->AdvancePicture();
							break;
					}

					break;
				}
			}
		}

		Uint64 currentTime = SDL_GetPerformanceCounter();
		double deltaSeconds = (currentTime - previousTime) / (double)SDL_GetPerformanceFrequency();
		previousTime = currentTime;

		game->Update(deltaSeconds);
		game->Render();
	}

	delete game;
	game = nullptr;

	SDL_JoystickClose(joystick);
	Audio::Dispose();
	Renderer::Dispose();
	WHBUnmountSdCard();
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
}
