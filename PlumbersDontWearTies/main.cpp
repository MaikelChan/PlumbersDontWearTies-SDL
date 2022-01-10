#include "main.h"
#include "config.h"

#include <iostream>

constexpr char* BASE_DATA_PATH = "Data/";

int main(int argc, char** args)
{
	// Initialize SDL

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0)
	{
		SDL_LogCritical(0, "Error initializing SDL: %s", SDL_GetError());
		return EXIT_FAILURE;
	}

	// Create window

	std::string title = "Plumbers Don't Wear Ties - v";
	SDL_Window* window = SDL_CreateWindow(title.append(PROJECT_VER).c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 960, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	if (window == nullptr)
	{
		SDL_LogCritical(0, "Could not create a window: %s", SDL_GetError());
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Initialize renderer

	if (!Renderer::Initialize(window, std::string(BASE_DATA_PATH) + "Font.ttf"))
	{
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Initialize game controller

	controller = nullptr;
	controllerInstanceID = -1;
	OpenFirstAvailableController();

	// Initialize the game

	Game* game = new Game(BASE_DATA_PATH);
	game->Start();

	Uint64 previousTime = SDL_GetPerformanceCounter();
	int16_t previousControllerYAxis = 0;

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
				case SDL_KEYDOWN:
				{
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							game->Stop();
							break;
						case SDLK_1:
						case SDLK_KP_1:
							game->SelectDecision(0);
							break;
						case SDLK_2:
						case SDLK_KP_2:
							game->SelectDecision(1);
							break;
						case SDLK_3:
						case SDLK_KP_3:
							game->SelectDecision(2);
							break;
						case SDLK_DOWN:
							game->SelectNextDecision();
							break;
						case SDLK_UP:
							game->SelectPreviousDecision();
							break;
						case SDLK_SPACE:
							game->AdvancePicture();
							break;
						case SDLK_RETURN:
							if (event.key.keysym.mod & KMOD_ALT)
							{
								ToggleFullscreen(window);
							}
							break;
					}

					break;
				}
				case SDL_CONTROLLERBUTTONDOWN:
				{
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_BACK:
							game->Stop();
							break;
						case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
							game->SelectNextDecision();
							break;
						case SDL_CONTROLLER_BUTTON_DPAD_UP:
							game->SelectPreviousDecision();
							break;
						case SDL_CONTROLLER_BUTTON_A:
							game->AdvancePicture();
							break;
						case SDL_CONTROLLER_BUTTON_START:
							ToggleFullscreen(window);
							break;
					}

					break;
				}
				case SDL_CONTROLLERAXISMOTION:
				{
					switch (event.caxis.axis)
					{
						case SDL_CONTROLLER_AXIS_LEFTY:
							if (previousControllerYAxis <= 24000 && event.caxis.value > 24000)
								game->SelectNextDecision();
							else if (previousControllerYAxis >= -24000 && event.caxis.value < -24000)
								game->SelectPreviousDecision();

							previousControllerYAxis = event.caxis.value;

							break;
					}
				}
				case SDL_CONTROLLERDEVICEADDED:
				{
					OpenFirstAvailableController();
					break;
				}
				case SDL_CONTROLLERDEVICEREMOVED:
				{
					if (event.cdevice.which == controllerInstanceID && controller != nullptr)
					{
						SDL_GameControllerClose(controller);
						controller = nullptr;
						controllerInstanceID = -1;

						SDL_Log("Controller has been disconnected: instance ID %i", event.cdevice.which);

						OpenFirstAvailableController();
					}

					break;
				}
				case SDL_WINDOWEVENT:
				{
					switch (event.window.event)
					{
						case SDL_WINDOWEVENT_SIZE_CHANGED:
							Renderer::WindowSizeChanged(event.window.data1, event.window.data2);
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

	if (controller != nullptr)
	{
		SDL_GameControllerClose(controller);
		controller = nullptr;
	}

	Renderer::Dispose();
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
}

void ToggleFullscreen(SDL_Window* window)
{
	bool isFullscreen = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP;

	if (isFullscreen)
	{
		if (SDL_SetWindowFullscreen(window, 0) < 0)
			SDL_LogError(0, "Can't set the game to window mode: %s", SDL_GetError());
	}
	else
	{
		if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) < 0)
			SDL_LogError(0, "Can't set the game to full screen mode: %s", SDL_GetError());
	}
}

void OpenFirstAvailableController()
{
	if (controller != nullptr) return;

	for (int j = 0; j < SDL_NumJoysticks(); j++)
	{
		if (!SDL_IsGameController(j)) continue;

		controller = SDL_GameControllerOpen(j);
		if (controller != nullptr)
		{
			SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);
			controllerInstanceID = SDL_JoystickInstanceID(joystick);

			SDL_Log("Found new controller: index %i, instance ID %i, name %s", j, controllerInstanceID, SDL_GameControllerName(controller));

			return;
		}
	}
}
