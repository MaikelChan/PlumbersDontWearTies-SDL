#include "Log.h"

#include <SDL.h>

void Log::Print(LogTypes type, const char* message, ...)
{
	va_list args;
	va_start(args, message);

	switch (type)
	{
		case LogTypes::Info:
			SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, message, args);
			break;
		case LogTypes::Warning:
			SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, message, args);
			break;
		case LogTypes::Error:
			SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, message, args);
			break;
		case LogTypes::Critical:
			SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_CRITICAL, message, args);
			break;
	}

	va_end(args);
}
