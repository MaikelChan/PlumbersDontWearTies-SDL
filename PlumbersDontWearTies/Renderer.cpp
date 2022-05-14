#include "Renderer.h"

#include "Log.h"

SDL_Window* Renderer::window = nullptr;
SDL_Renderer* Renderer::renderer = nullptr;

int32_t Renderer::rendererWidth = 0;
int32_t Renderer::rendererHeight = 0;
SDL_Rect Renderer::viewportRect = {};
float Renderer::viewportScale = 0;

SDL_Texture* Renderer::currentTexture = nullptr;
int32_t Renderer::currentTextureWidth = 0;
int32_t Renderer::currentTextureHeight = 0;

TTF_Font* Renderer::textFont = nullptr;
SDL_Texture* Renderer::currentTextTexture = nullptr;
int32_t Renderer::currentTextTextureWidth = 0;
int32_t Renderer::currentTextTextureHeight = 0;

bool Renderer::Initialize(SDL_Window* window, const std::string fontPath)
{
	if (IsInitialized()) return false;

	// Initialize SDL renderer

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == nullptr)
	{
		Log::Print(LogTypes::Critical, "Could not create a renderer: %s", SDL_GetError());
		return false;
	}

	// Set initial resolution

	int rw, rh;
	if (SDL_GetRendererOutputSize(renderer, &rw, &rh) < 0)
	{
		Dispose();
		Log::Print(LogTypes::Critical, "Could not get renderer output size: %s", SDL_GetError());
		return false;
	}

	Log::Print(LogTypes::Info, "Renderer initialized: resolution %ix%i.", rw, rh);

	WindowSizeChanged(rw, rh);

	// Load font

	if (TTF_Init() < 0)
	{
		Log::Print(LogTypes::Error, "TTF has not been initialized: %s", TTF_GetError());
	}

	textFont = TTF_OpenFont(fontPath.c_str(), 48);
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
		SDL_DestroyTexture(currentTexture);
		currentTexture = nullptr;
	}

	if (currentTextTexture != nullptr)
	{
		SDL_DestroyTexture(currentTextTexture);
		currentTextTexture = nullptr;
	}

	if (renderer != nullptr)
	{
		SDL_DestroyRenderer(renderer);
		renderer = nullptr;
	}
}

void Renderer::Clear(const uint8_t r, const uint8_t g, const uint8_t b)
{
	if (!IsInitialized()) return;

	SDL_SetRenderDrawColor(renderer, r, g, b, 255);
	SDL_RenderClear(renderer);
}

void Renderer::RenderPicture()
{
	if (!IsInitialized()) return;

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	SDL_RenderCopy(renderer, currentTexture, NULL, NULL);
}

void Renderer::RenderDecisionSelection(const int32_t selectionX, const int32_t selectionY, const int32_t selectionW, const int32_t selectionH)
{
	if (!IsInitialized()) return;

	float totalSeconds = SDL_GetPerformanceCounter() / (float)SDL_GetPerformanceFrequency();
	uint8_t alpha = static_cast<uint8_t>((sin(totalSeconds * M_PI * 2) * 0.25 + 0.75) * 255);

	SDL_Rect selectionRect = { selectionX,  selectionY, selectionW,  selectionH };
	ScaleRect(&selectionRect, viewportScale);

	SDL_SetRenderDrawColor(renderer, alpha, alpha, alpha, 255);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_MOD);
	SDL_RenderFillRect(renderer, &selectionRect);
}

void Renderer::RenderScore()
{
	if (!IsInitialized()) return;

	float textScale = viewportScale * 0.5f;

	SDL_Rect textRect;
	textRect.x = 32;
	textRect.y = static_cast<int32_t>(viewportRect.h / textScale) - 32 - currentTextTextureHeight;
	textRect.w = currentTextTextureWidth;
	textRect.h = currentTextTextureHeight;
	ScaleRect(&textRect, textScale);

	SDL_RenderCopy(renderer, currentTextTexture, NULL, &textRect);
}

void Renderer::Present()
{
	SDL_RenderPresent(renderer);
}

void Renderer::WindowSizeChanged(const int32_t width, const int32_t height)
{
	if (!IsInitialized()) return;

	rendererWidth = width;
	rendererHeight = height;

	UpdateViewport();
	Log::Print(LogTypes::Info, "New window size: %ix%i.", width, height);
}

bool Renderer::LoadPictureFromBMP(const std::string baseDataPath, std::string fileName)
{
	if (!IsInitialized()) return false;

	if (currentTexture != nullptr)
	{
		SDL_DestroyTexture(currentTexture);
		currentTexture = nullptr;
	}

	SDL_Surface* newSurface = SDL_LoadBMP((baseDataPath + fileName).c_str());

	if (newSurface == nullptr)
	{
		Log::Print(LogTypes::Error, "Can't load bitmap: %s", SDL_GetError());
		return false;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_Texture* newTexture = SDL_CreateTextureFromSurface(renderer, newSurface);

	if (newTexture == nullptr)
	{
		SDL_FreeSurface(newSurface);
		Log::Print(LogTypes::Error, "Can't create texture: %s", SDL_GetError());
		return false;
	}

	currentTextureWidth = newSurface->w;
	currentTextureHeight = newSurface->h;
	currentTexture = newTexture;

	SDL_FreeSurface(newSurface);

	UpdateViewport();

	Log::Print(LogTypes::Info, "Loaded picture %s (%ix%i)", fileName.c_str(), currentTextureWidth, currentTextureHeight);

	return true;
}

bool Renderer::GenerateScoreText(const std::string text)
{
	if (!IsInitialized()) return false;

	if (currentTextTexture != nullptr)
	{
		SDL_DestroyTexture(currentTextTexture);
		currentTextTexture = nullptr;
		currentTextTextureWidth = 0;
		currentTextTextureHeight = 0;
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

	SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

	SDL_FreeSurface(textSurface);

	if (textTexture == nullptr)
	{
		Log::Print(LogTypes::Error, "Can't create text texture: %s", SDL_GetError());
		return false;
	}

	int32_t w, h;
	if (TTF_SizeText(textFont, cText, &w, &h) < 0)
	{
		SDL_DestroyTexture(textTexture);
		Log::Print(LogTypes::Error, "Can't calculate size of text texture: %s", TTF_GetError());
		return false;
	}

	currentTextTexture = textTexture;
	currentTextTextureWidth = w;
	currentTextTextureHeight = h;

	return true;
}

void Renderer::UpdateViewport()
{
	float rendererAspectRatio = static_cast<float>(rendererWidth) / rendererHeight;
	float textureAspectRatio = static_cast<float>(currentTextureWidth) / currentTextureHeight;

	if (rendererAspectRatio > textureAspectRatio)
	{
		int32_t gameWidth = static_cast<int32_t>(rendererHeight * textureAspectRatio);
		viewportRect.x = (rendererWidth - gameWidth) >> 1;
		viewportRect.y = 0;
		viewportRect.w = gameWidth;
		viewportRect.h = rendererHeight;

		viewportScale = static_cast<float>(rendererHeight) / currentTextureHeight;
	}
	else
	{
		int32_t gameHeight = static_cast<int32_t>(rendererWidth / textureAspectRatio);
		viewportRect.x = 0;
		viewportRect.y = (rendererHeight - gameHeight) >> 1;
		viewportRect.w = rendererWidth;
		viewportRect.h = gameHeight;

		viewportScale = static_cast<float>(rendererWidth) / currentTextureWidth;
	}

	SDL_RenderSetViewport(renderer, &viewportRect);
}

void Renderer::ScaleRect(SDL_Rect* rectToScale, const float scale)
{
	rectToScale->x = static_cast<int32_t>(rectToScale->x * scale);
	rectToScale->y = static_cast<int32_t>(rectToScale->y * scale);
	rectToScale->w = static_cast<int32_t>(rectToScale->w * scale);
	rectToScale->h = static_cast<int32_t>(rectToScale->h * scale);
}
