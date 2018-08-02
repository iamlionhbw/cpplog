#include "logging.h"

#include <stdarg.h>

void SetAsyncLog(bool bFlag)
{
	extern bool __ASYNC_LOG;
	__ASYNC_LOG = bFlag;
}

void AppendConsoleLog(const char* pcszLoggerName, SuLogLevel sllLevel)
{
	SuLog::GetLogMgr()->AddLogger(pcszLoggerName, new ConsoleLogConfig(sllLevel));
}

void TraceLog(const char* pcszLoggerName, SuLogLevel sllLevel, const char* pcszFile, const char* pcszFunc, long lLine, const char* format, ...)
{
	static const int BUFSIZE = 8192;
	char buf[BUFSIZE] = { 0 };
	memset(buf, 0, BUFSIZE);
	va_list args;
	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);
	SuLog::GetLogMgr()->TraceLog(pcszLoggerName, sllLevel, pcszFile, pcszFunc, lLine, buf);
}