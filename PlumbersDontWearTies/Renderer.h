#pragma once

#include <string>

#include <SDL.h>
#include <SDL_ttf.h>

class Renderer
{
private:
	static SDL_Window* window;
	static SDL_Renderer* renderer;

	static int32_t rendererWidth;
	static int32_t rendererHeight;
	static SDL_Rect viewportRect;
	static float viewportScale;

	static SDL_Texture* currentTexture;
	static int32_t currentTextureWidth;
	static int32_t currentTextureHeight;

	static TTF_Font* textFont;
	static SDL_Texture* currentTextTexture;
	static int32_t currentTextTextureWidth;
	static int32_t currentTextTextureHeight;

public:
	static bool Initialize(SDL_Window* window, const std::string fontPath);
	static void Dispose();

	static void Clear(const uint8_t r, const uint8_t g, const uint8_t b);
	static void RenderPicture();
	static void RenderDecisionSelection(const int32_t selectionX, const int32_t selectionY, const int32_t selectionW, const int32_t selectionH);
	static void RenderScore();
	static void Present();

	static void WindowSizeChanged(const int32_t width, const int32_t height);

	static bool LoadPictureFromBMP(const std::string baseDataPath, const std::string fileName);
	static bool GenerateScoreText(const std::string text);

	inline static bool IsInitialized() { return renderer != nullptr; }

private:
	static void UpdateViewport();
	static void ScaleRect(SDL_Rect* rectToScale, const float scale);
};
