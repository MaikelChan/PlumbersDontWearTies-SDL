#include "Renderer.h"

#include <cmath>

#include "Log.h"

SDL_Surface* Renderer::screenSurface = nullptr;

SDL_Rect Renderer::viewportRect = {};

SDL_Surface* Renderer::currentTexture = nullptr;
SDL_Surface* Renderer::selectionSurface = nullptr;

TTF_Font* Renderer::textFont = nullptr;
SDL_Surface* Renderer::currentTextTexture = nullptr;

bool Renderer::Initialize(const std::string fontPath)
{
	if (IsInitialized()) return false;

	// Initialize SDL video

	screenSurface = SDL_SetVideoMode(640, 480, 16, /*SDL_DOUBLEBUF |*/ SDL_FITHEIGHT | SDL_CONSOLEBOTTOM);
	if (screenSurface == nullptr)
	{
		Log::Print(LogTypes::Critical, "Unable to set video mode: %s", SDL_GetError());
		return false;
	}

	Log::Print(LogTypes::Info, "Renderer initialized: res %ix%i.", screenSurface->w, screenSurface->h);

	// Create surface for drawing the selection rect
	// Alpha blending apparently can only be applied to entire surfaces in SDL1.

	selectionSurface = SDL_CreateRGBSurface(0 /*SDL_SWSURFACE | SDL_ANYFORMAT*/, screenSurface->w, screenSurface->h, screenSurface->format->BitsPerPixel, 0, 0, 0, 0);

	if (selectionSurface == nullptr)
	{
		Log::Print(LogTypes::Critical, "Can't create selection surface: %s", SDL_GetError());
		return false;
	}

	SDL_FillRect(selectionSurface, NULL, SDL_MapRGB(screenSurface->format, 0, 0, 0));

	// Load font

	if (TTF_Init() < 0)
	{
		Log::Print(LogTypes::Error, "TTF has not been initialized: %s", TTF_GetError());
	}

	textFont = TTF_OpenFont(fontPath.c_str(), 28);
	if (textFont == nullptr)
	{
		Log::Print(LogTypes::Error, "%s has not been found or couldn't be opened: %s", fontPath.c_str(), TTF_GetError());
	}

	return true;
}

void Renderer::Dispose()
{
	if (textFont != nullptr)
	{
		TTF_CloseFont(textFont);
		textFont = nullptr;
	}

	if (TTF_WasInit())
	{
		TTF_Quit();
	}

	if (currentTexture != nullptr)
	{
		SDL_FreeSurface(currentTexture);
		currentTexture = nullptr;
	}

	if (selectionSurface != nullptr)
	{
		SDL_FreeSurface(selectionSurface);
		selectionSurface = nullptr;
	}

	if (currentTextTexture != nullptr)
	{
		SDL_FreeSurface(currentTextTexture);
		currentTextTexture = nullptr;
	}
}

void Renderer::Clear(const uint8_t r, const uint8_t g, const uint8_t b)
{
	if (!IsInitialized()) return;

	SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0, 0, 0));
}

void Renderer::RenderPicture()
{
	if (!IsInitialized()) return;

	SDL_BlitSurface(currentTexture, NULL, screenSurface, &viewportRect);
}

void Renderer::RenderDecisionSelection(const int16_t selectionX, const int16_t selectionY, const uint16_t selectionW, const uint16_t selectionH)
{
	if (!IsInitialized()) return;

	Uint32 currentTime = SDL_GetTicks();
	double totalSeconds = currentTime / 1000.0;

	uint8_t alpha = static_cast<uint8_t>((sin(totalSeconds * M_PI * 2) * 0.25 + 0.25) * 255);

	SDL_Rect selectionRect = { static_cast<int16_t>(viewportRect.x + selectionX), selectionY, selectionW, selectionH };

	SDL_SetAlpha(selectionSurface, SDL_SRCALPHA, alpha);
	SDL_BlitSurface(selectionSurface, &selectionRect, screenSurface, &selectionRect);
}

void Renderer::RenderScore()
{
	if (!IsInitialized()) return;

	SDL_Rect textRect;
	textRect.x = viewportRect.x + 28;
	textRect.y = viewportRect.h - 28 - currentTextTexture->h;
	textRect.w = currentTextTexture->w;
	textRect.h = currentTextTexture->h;

	SDL_BlitSurface(currentTextTexture, NULL, screenSurface, &textRect);
}

void Renderer::Present()
{
	SDL_Flip(screenSurface);
}

bool Renderer::LoadPictureFromBMP(const std::string baseDataPath, std::string fileName)
{
	if (!IsInitialized()) return false;

	if (currentTexture != nullptr)
	{
		SDL_FreeSurface(currentTexture);
		currentTexture = nullptr;
	}

	SDL_Surface* newSurface = SDL_LoadBMP((baseDataPath + fileName).c_str());

	if (newSurface == nullptr)
	{
		Log::Print(LogTypes::Error, "Can't load bitmap: %s", SDL_GetError());
		return false;
	}
	
	currentTexture = newSurface;
	UpdateViewport();

	Log::Print(LogTypes::Info, "Loaded picture %s (%ix%i)", fileName.c_str(), currentTexture->w, currentTexture->h);

	return true;
}

bool Renderer::GenerateScoreText(const std::string text)
{
	if (!IsInitialized()) return false;

	if (currentTextTexture != nullptr)
	{
		SDL_FreeSurface(currentTextTexture);
		currentTextTexture = nullptr;
	}

	if (text.empty())
	{
		return true;
	}

	const char* cText = text.c_str();

	if (textFont == nullptr)
	{
		Log::Print(LogTypes::Info, "%s", cText);
		return false;
	}

	SDL_Color white = { 255, 255, 255, 255 };
	SDL_Surface* textSurface = TTF_RenderText_Blended(textFont, cText, white);

	if (textSurface == nullptr)
	{
		Log::Print(LogTypes::Error, "Can't create text surface: %s", SDL_GetError());
		return false;
	}

	currentTextTexture = textSurface;

	return true;
}

void Renderer::UpdateViewport()
{
	float rendererAspectRatio = static_cast<float>(screenSurface->w) / screenSurface->h;
	float textureAspectRatio = static_cast<float>(currentTexture->w) / currentTexture->h;

	if (rendererAspectRatio > textureAspectRatio)
	{
		int32_t gameWidth = static_cast<int32_t>(screenSurface->h * textureAspectRatio);
		viewportRect.x = (screenSurface->w - gameWidth) >> 1;
		viewportRect.y = 0;
		viewportRect.w = gameWidth;
		viewportRect.h = screenSurface->h;
	}
	else
	{
		int32_t gameHeight = static_cast<int32_t>(screenSurface->w / textureAspectRatio);
		viewportRect.x = 0;
		viewportRect.y = (screenSurface->h - gameHeight) >> 1;
		viewportRect.w = screenSurface->w;
		viewportRect.h = gameHeight;
	}
}
