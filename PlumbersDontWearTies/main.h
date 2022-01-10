#pragma once

#include <SDL.h>

SDL_GameController* controller;
SDL_JoystickID controllerInstanceID;

int main(int argc, char** args);
void ToggleFullscreen(SDL_Window* window);
void OpenFirstAvailableController();
