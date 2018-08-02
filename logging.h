#ifndef _LOGGING_H_
#define _LOGGING_H_

#pragma once

#include "sulog.h"

#define SLOG_DEBUG(...) TraceLog("_ROOT_", SuLogLevel::DEBUG, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define SLOG_INFO(...) TraceLog("_ROOT_", SuLogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define SLOG_WARN(...) TraceLog("_ROOT_", SuLogLevel::WARN, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define SLOG_ERROR(...) TraceLog("_ROOT_", SuLogLevel::ERR, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define SLOG_FATAL(...) TraceLog("_ROOT_", SuLogLevel::FATAL, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#define SLOG_iDEBUG(pcszLoggerName, ...) TraceLog(pcszLoggerName, SuLogLevel::DEBUG, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

void SetAsyncLog(bool bFlag);

void AppendConsoleLog(const char* pcszLoggerName, SuLogLevel sllLevel);

void TraceLog(const char* pcszLoggerName, SuLogLevel sllLevel, const char* pcszFile, const char* pcszFunc, long lLine, const char* format, ...);

#endif // !_LOGGING_H_