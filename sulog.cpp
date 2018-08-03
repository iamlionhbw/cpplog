#include "sulog.h"

#include <climits>
#include <thread>
#include <algorithm>

#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

namespace sulog
{
	bool Logger::cls_pub_async = true;

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
		const char* pcszLogMsg) :
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
		const int BUFSIZE = 4096;
		char buf[BUFSIZE] = { 0 };
		sprintf(
			buf, "%u-%02u-%02u %02u:%02u:%02u.%03u %s (%s -> %d) [%s]: %s",
			m_stLogTime.wYear, m_stLogTime.wMonth,
			m_stLogTime.wDay, m_stLogTime.wHour,
			m_stLogTime.wMinute, m_stLogTime.wSecond,
			m_stLogTime.wMilliseconds, m_strLogFilePath.c_str(),
			m_strLogFunc.c_str(), m_lLogLine,
			GetLevelStr(m_sllMsgLevel), m_strLogMsg.c_str()
		);
		return buf;
	}

	LogHandler::LogHandler(SuLogLevel sllLevel) :m_sllLevel(sllLevel)
	{}

	ConsoleLogHandler::ConsoleLogHandler(SuLogLevel sllLevel) :
		LogHandler(sllLevel)
	{}

	void ConsoleLogHandler::WriteLog(LogMessage &msg)
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

	FileLogHandler::FileLogHandler(const char* pcszLogFile, SuLogLevel sllOutputLevel) :
		LogHandler(sllOutputLevel), m_fp(NULL), m_pcszLogFile(pcszLogFile)
	{
		//if (PathFileExistsA(pcszLogFile))
		{
			m_fp = fopen(m_pcszLogFile, "a+");
		}
	}

	FileLogHandler::~FileLogHandler()
	{
		if (m_fp)
		{
			fclose(m_fp);
			m_fp = NULL;
		}
	}

	void FileLogHandler::WriteLog(LogMessage &msg)
	{
		if (msg.MarkOutput(m_sllLevel) && m_fp)
		{
			fprintf(m_fp, "%s\n", msg.ToString().c_str());
			fflush(m_fp);
		}
	}

	DailyFileLogHandler::DailyFileLogHandler(
		const char* pcszLogFile, SuLogLevel sllOutputLevel):
		FileLogHandler(pcszLogFile, sllOutputLevel)
	{}

	void DailyFileLogHandler::WriteLog(LogMessage &msg)
	{
		if (m_fp)
		{
			__RotateCheckProcedure();
		}
		FileLogHandler::WriteLog(msg);
	}

	void DailyFileLogHandler::__RotateCheckProcedure()
	{
		FILETIME lft;
		SYSTEMTIME createTime, currentTime;
		WIN32_FILE_ATTRIBUTE_DATA wfad;
		GetFileAttributesExA(m_pcszLogFile, GetFileExInfoStandard, &wfad);
		FileTimeToLocalFileTime(&wfad.ftCreationTime, &lft);
		FileTimeToSystemTime(&lft, &createTime);
		GetLocalTime(&currentTime);
		if (currentTime.wYear > createTime.wYear || 
			((!currentTime.wYear ^ createTime.wYear) &&
				currentTime.wMonth > createTime.wMonth) ||
			((!currentTime.wYear ^ createTime.wYear) && 
			(!currentTime.wMonth ^ createTime.wMonth) &&
				currentTime.wDay > createTime.wDay)
			)
		{
			fclose(m_fp);
			std::string logName(m_pcszLogFile);
			char date[16] = { 0 };
			sprintf(date, "%04u%02u%02u", createTime.wYear, createTime.wMonth, createTime.wDay);
			logName += date;
			rename(m_pcszLogFile, logName.c_str());
			m_fp = fopen(m_pcszLogFile, "a+");
		}

	}

	Logger::Logger(std::string strName) :m_strName(strName), m_hSem(NULL), m_bExit(false)
	{
		m_hSem = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
		std::thread tmp(Logger::__WriteLogThreadFunc, this);
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

	void Logger::AddHandler(LogHandler *pLogHdl)
	{
		m_vecHdl.push_back(pLogHdl);
	}

	void Logger::PushLog(LogMessage &msg)
	{
		this->__Push(msg);
		ReleaseSemaphore(m_hSem, 1, NULL);
	}

	void Logger::PushLog(
		SuLogLevel sllLevel, const char* pcszFile,
		const char* pcszFunc, long lLine,
		const char* pcszMsgContent)
	{
		if (Logger::cls_pub_async)
		{
			this->__Push(sllLevel, pcszFile, pcszFunc, lLine, pcszMsgContent);
			ReleaseSemaphore(m_hSem, 1, NULL);
		}
		else
		{
			LogMessage msg(sllLevel, pcszFile, pcszFunc, lLine, pcszMsgContent);
			for (auto &pHdl : m_vecHdl)
			{
				pHdl->WriteLog(msg);
			}
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

	bool Logger::Idle()
	{
		return m_qMsgPump.empty();
	}

	void Logger::__WriteLogThreadFunc(Logger *pself)
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
			for (auto &pHdl : pself->m_vecHdl)
			{
				pHdl->WriteLog(msg);
			}
		}
	}

	void Logger::__Push(LogMessage &msg)
	{
		std::lock_guard<std::mutex> lg(m_mtxMsgPump);
		m_qMsgPump.push(msg);
	}

	void Logger::__Push(
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
		this->AddLogger("_ROOT_", NULL);
	}

	SuLog::~SuLog()
	{
		this->FlushAllLogger();
		for (auto &plog : m_vecLogger)
		{
			plog->MarkExit();
			plog->PushLog(LogMessage());
		}
		std::for_each(
			m_vecLogger.begin(),
			m_vecLogger.end(),
			[](Logger* plog) {
			delete plog;
		});
	}

	void SuLog::SetAsyncLog(bool bAsyncLogging)
	{
		Logger::cls_pub_async = bAsyncLogging;
	}

	void SuLog::AddLogger(const std::string &strLoggerName, LogHandler *pHdl)
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
		if (pHdl)
		{
			(*itLogger)->AddHandler(pHdl);
		}
	}

	void SuLog::FlushAllLogger()
	{
		if (Logger::cls_pub_async)
		{
			while (true)
			{
				auto itLogger = std::find_if(
					m_vecLogger.begin(),
					m_vecLogger.end(),
					[](Logger *plog) {
					if (!plog->Idle())
					{
						return true;
					}
					return false;
				});
				if (itLogger == m_vecLogger.end())
					break;
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
		}
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

	void AppendConsoleLog(SuLogLevel sllLevel)
	{
		AppendConsoleLog(MAIN_LOGGER, sllLevel);
	}

	void AppendConsoleLog(const char* pcszLoggerName, SuLogLevel sllLevel)
	{
		SuLog::GetLogMgr()->AddLogger(pcszLoggerName, new ConsoleLogHandler(sllLevel));
	}

	void AppendFileLog(const char* pcszFileLog, SuLogLevel sllLevel, bool bDaily)
	{
		AppendFileLog(MAIN_LOGGER, pcszFileLog, sllLevel, bDaily);
	}

	void AppendFileLog(const char* pcszLoggerName, const char* pcszFileLog, SuLogLevel sllLevel, bool bDaily)
	{
		SuLog::GetLogMgr()->AddLogger(
			pcszLoggerName,
			bDaily ? new DailyFileLogHandler(pcszFileLog, sllLevel) : new FileLogHandler(pcszFileLog, sllLevel)
		);
	}

	void TraceLog(const char* pcszLoggerName, SuLogLevel sllLevel, const char* pcszFile, const char* pcszFunc, long lLine, const char* format, ...)
	{
		static const int BUFSIZE = 4096;
		char buf[BUFSIZE] = { 0 };
		memset(buf, 0, BUFSIZE);
		va_list args;
		va_start(args, format);
		vsprintf(buf, format, args);
		va_end(args);
		SuLog::GetLogMgr()->TraceLog(pcszLoggerName, sllLevel, pcszFile, pcszFunc, lLine, buf);
	}

	LogGuard::LogGuard(bool bAsyncLogging)
	{
		SuLog::GetLogMgr()->SetAsyncLog(bAsyncLogging);
	}

	LogGuard::~LogGuard()
	{
		this->__FlushAllLogger();
	}

	void LogGuard::__FlushAllLogger()
	{
		SuLog::GetLogMgr()->FlushAllLogger();
	}
}