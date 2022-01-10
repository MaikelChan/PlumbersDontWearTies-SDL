#pragma once

#include <string>

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

class Renderer
{
private:
	static SDL_Surface* screenSurface;

	static SDL_Rect viewportRect;

	static SDL_Surface* currentTexture;
    static SDL_Surface* selectionSurface;

	static TTF_Font* textFont;
	static SDL_Surface* currentTextTexture;

public:
	static bool Initialize(const std::string fontPath);
	static void Dispose();

	static void Clear(const uint8_t r, const uint8_t g, const uint8_t b);
	static void RenderPicture();
	static void RenderDecisionSelection(const int16_t selectionX, const int16_t selectionY, const uint16_t selectionW, const uint16_t selectionH);
	static void RenderScore();
	static void Present();

	static bool LoadPictureFromBMP(const std::string baseDataPath, const std::string fileName);
	static bool GenerateScoreText(const std::string text);

	inline static bool IsInitialized() { return screenSurface != nullptr; }

private:
	static void UpdateViewport();
};
