#pragma once

enum class LogTypes
{
	Info,
	Warning,
	Error,
	Critical
};

class Log
{
public:
	static void Print(LogTypes type, const char* message, ...);
};
