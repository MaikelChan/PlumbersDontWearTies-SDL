#include "Log.h"

#include <cstring>
#include <cstdarg>
#include <cstdio>

void Log::Print(LogTypes type, const char* message, ...)
{
	va_list args;
	va_start(args, message);

	char newMessage[512];

	switch (type)
	{
		case LogTypes::Info:
			strcpy(newMessage, "");
			break;
		case LogTypes::Warning:
			strcpy(newMessage, "WARN: ");
			break;
		case LogTypes::Error:
			strcpy(newMessage, "ERROR: ");
			break;
		case LogTypes::Critical:
			strcpy(newMessage, "CRITICAL: ");
			break;
	}

	strcat(newMessage, message);
    strcat(newMessage, "\n");
	vprintf(newMessage, args);
	va_end(args);
}
