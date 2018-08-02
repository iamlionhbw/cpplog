#include "sulog.h"

#include <climits>
#include <thread>
#include <algorithm>

#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

extern bool __ASYNC_LOG = true;

const char* GetLevelStr(SuLogLevel level)
{
	switch (level)
	{
	case SuLogLevel::DEBUG:
		return "DEBUG";
	case SuLogLevel::INFO:
		return "INFO";
	case SuLogLevel::WARN:
		return "WARN";
	case SuLogLevel::ERR:
		return "ERROR";
	default:
		return "FATAL";
	}
}

LogMessage::LogMessage() {}

LogMessage::LogMessage(SuLogLevel level, const char* pcszLogFile,
	const char* pcszLogFunc, long lLine,
	const char* pcszLogMsg):
	m_sllMsgLevel(level), m_strLogFilePath(pcszLogFile),
	m_strLogFunc(pcszLogFunc), m_lLogLine(lLine),
	m_strLogMsg(pcszLogMsg)
{
	GetLocalTime(&m_stLogTime);
}

bool LogMessage::MarkOutput(SuLogLevel sllCurrentLevel)
{
	if (sllCurrentLevel <= m_sllMsgLevel)
		return true;
	return false;
}

std::string LogMessage::ToString()
{
	const int BUFSIZE = 8192;
	char buf[BUFSIZE] = { 0 };
	sprintf(
		buf, "%u-%02u-%02u %02u:%02u:%02u.%03u %s(%s -> %d) [%s]: %s",
		m_stLogTime.wYear, m_stLogTime.wMonth,
		m_stLogTime.wDay, m_stLogTime.wHour,
		m_stLogTime.wMinute, m_stLogTime.wSecond,
		m_stLogTime.wMilliseconds, m_strLogFilePath.c_str(),
		m_strLogFunc.c_str(), m_lLogLine,
		GetLevelStr(m_sllMsgLevel), m_strLogMsg.c_str()
	);
	return buf;
}

LogConfig::LogConfig(SuLogLevel sllLevel) :m_sllLevel(sllLevel)
{}

ConsoleLogConfig::ConsoleLogConfig(SuLogLevel sllLevel) :
	LogConfig(sllLevel)
{}

void ConsoleLogConfig::WriteLog(LogMessage &msg)
{
	// All console output share the same mutex
	static std::mutex mtxConsoleSerial;
	if (msg.MarkOutput(m_sllLevel))
	{
		std::lock_guard<std::mutex> lg(mtxConsoleSerial);
#if _CONSOLE
		printf("%s\n", msg.ToString().c_str());
#elif _WINDOW
		OutputDebugStringA(msg.ToString().c_str());
#endif
	}
}

FileLogConfig::FileLogConfig(const char* pcszLogFile, SuLogLevel sllOutputLevel):
	LogConfig(sllOutputLevel), m_fp(NULL), m_pcszLogFile(pcszLogFile)
{
	if (PathFileExistsA(pcszLogFile))
	{

	}
}

void FileLogConfig::WriteLog(LogMessage &msg)
{

}

Logger::Logger(std::string strName) :m_strName(strName), m_hSem(NULL), m_bExit(false)
{
	m_hSem = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
	std::thread tmp(Logger::_WriteLogThreadFunc, this);
	tmp.detach();
}

Logger::~Logger()
{
	for (auto it = m_vecHdl.begin(); it != m_vecHdl.end(); ++it)
	{
		delete(*it);
	}
	m_vecHdl.clear();
	if (m_hSem)
	{
		CloseHandle(m_hSem);
		m_hSem = NULL;
	}
}

void Logger::AddConfig(LogConfig *pLogConf)
{
	m_vecHdl.push_back(pLogConf);
}

void Logger::PushLog(LogMessage &msg)
{
	this->_Push(msg);
	ReleaseSemaphore(m_hSem, 1, NULL);
}

void Logger::PushLog(
	SuLogLevel sllLevel, const char* pcszFile,
	const char* pcszFunc, long lLine,
	const char* pcszMsgContent)
{
	if (__ASYNC_LOG)
	{
		this->_Push(sllLevel, pcszFile, pcszFunc, lLine, pcszMsgContent);
		ReleaseSemaphore(m_hSem, 1, NULL);
	}
	else
	{
		LogMessage msg(sllLevel, pcszFile, pcszFunc, lLine, pcszMsgContent);
		std::for_each(
			m_vecHdl.begin(),
			m_vecHdl.end(),
			[&msg](LogConfig *plc) {
			plc->WriteLog(msg);
		});
	}
}

void Logger::MarkExit()
{
	m_bExit = true;
}

std::string Logger::Name()
{
	return m_strName;
}

void Logger::_WriteLogThreadFunc(Logger *pself)
{
	while (true)
	{
		WaitForSingleObject(pself->m_hSem, INFINITE);
		if (pself->m_bExit)
		{
			break;
		}
		// Do Log
		auto msg = pself->_Pop();
		std::for_each(
			pself->m_vecHdl.begin(),
			pself->m_vecHdl.end(),
			[&msg](LogConfig *plc) {
			plc->WriteLog(msg);
		});
	}
}

void Logger::_Push(LogMessage &msg)
{
	std::lock_guard<std::mutex> lg(m_mtxMsgPump);
	m_qMsgPump.push(msg);
}

void Logger::_Push(
	SuLogLevel sllLevel, const char* pcszFile,
	const char* pcszFunc, long lLine,
	const char* pcszMsgContent)
{
	std::lock_guard<std::mutex> lg(m_mtxMsgPump);
	m_qMsgPump.emplace(sllLevel, pcszFile, pcszFunc, lLine, pcszMsgContent);
}

LogMessage Logger::_Pop()
{
	std::lock_guard<std::mutex> lg(m_mtxMsgPump);
	auto msg = m_qMsgPump.front();
	m_qMsgPump.pop();
	return msg;
}

static std::mutex mtxSingleton;
SuLog* SuLog::GetLogMgr()
{
	static SuLog* pself = NULL;
	if (!pself)
	{
		std::lock_guard<std::mutex> lg(mtxSingleton);
		if (!pself)
		{
			pself = new SuLog();
		}
	}
	return pself;
}

SuLog::SuLog()
{
	this->AddLogger("_ROOT_", new ConsoleLogConfig(SuLogLevel::DEBUG));
}

SuLog::~SuLog()
{
	printf("Bye\n");
	std::for_each(
		m_vecLogger.begin(),
		m_vecLogger.end(),
		[](Logger* plog) {
		plog->MarkExit();
		plog->PushLog(LogMessage());
	});
	std::for_each(
		m_vecLogger.begin(),
		m_vecLogger.end(),
		[](Logger* plog) {
		delete plog;
	});
}

void SuLog::AddLogger(const std::string &strLoggerName, LogConfig *plc)
{
	auto itLogger = std::find_if(m_vecLogger.begin(), m_vecLogger.end(), [&strLoggerName](Logger *plog) {
		return plog->Name() == strLoggerName;
	});
	if (itLogger == m_vecLogger.end())
	{
		m_vecLogger.push_back(new Logger(strLoggerName));
		itLogger = std::find_if(m_vecLogger.begin(), m_vecLogger.end(), [&strLoggerName](Logger* plog) {
			return plog->Name() == strLoggerName;
		});
	}
	(*itLogger)->AddConfig(plc);
}

void SuLog::TraceLog(
	const std::string &strLoggerName, SuLogLevel sllLevel,
	const char* pcszFile, const char* pcszFunc,
	long lLine, const char* pcszMsgContent
)
{
	auto itLogger = std::find_if(m_vecLogger.begin(), m_vecLogger.end(), [&strLoggerName](Logger* plog) {
		return plog->Name() == strLoggerName;
	});
	if (itLogger != m_vecLogger.end())
	{
		(*itLogger)->PushLog(
			sllLevel, pcszFile, pcszFunc, lLine, pcszMsgContent
		);
	}
}