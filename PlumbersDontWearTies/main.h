#pragma once

#include "Game.h"

// Button mapping:
// https://github.com/devkitPro/SDL/blob/switch-sdl2/src/joystick/switch/SDL_sysjoystick.c#L52

#define JOY_A      0
#define JOY_B      1
#define JOY_X      2
#define JOY_Y      3
#define JOY_PLUS   10
#define JOY_MINUS  11
#define DPAD_LEFT  12
#define DPAD_UP    13
#define DPAD_RIGHT 14
#define DPAD_DOWN  15
#define JOY_LEFT   16
#define JOY_UP     17
#define JOY_RIGHT  18
#define JOY_DOWN   19

int main(int argc, char** args);
