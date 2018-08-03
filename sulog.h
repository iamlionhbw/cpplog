#ifndef _SULOG_H_
#define _SULOG_H_

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <queue>
#include <vector>
#include <mutex>
#include <string>

#include <Windows.h>

#define ENABLE_LOG 1

namespace sulog
{
	static const char* MAIN_LOGGER = "_ROOT_";
}

#if ENABLE_LOG
#define SULOG_DEBUG(...) sulog::TraceLog(sulog::MAIN_LOGGER, sulog::SuLogLevel::DEBUG, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define SULOG_INFO(...) sulog::TraceLog(sulog::MAIN_LOGGER, sulog::SuLogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define SULOG_WARN(...) sulog::TraceLog(sulog::MAIN_LOGGER, sulog::SuLogLevel::WARN, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define SULOG_ERROR(...) sulog::TraceLog(sulog::MAIN_LOGGER, sulog::SuLogLevel::ERR, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define SULOG_FATAL(...) sulog::TraceLog(sulog::MAIN_LOGGER, sulog::SuLogLevel::FATAL, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#define SULOG_iDEBUG(pcszLoggerName, ...) sulog::TraceLog(pcszLoggerName, sulog::SuLogLevel::DEBUG, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define SULOG_iINFO(pcszLoggerName, ...) sulog::TraceLog(pcszLoggerName, sulog::SuLogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define SULOG_iWARN(pcszLoggerName, ...) sulog::TraceLog(pcszLoggerName, sulog::SuLogLevel::WARN, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define SULOG_iERROR(pcszLoggerName, ...) sulog::TraceLog(pcszLoggerName, sulog::SuLogLevel::ERR, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define SULOG_iFATAL(pcszLoggerName, ...) sulog::TraceLog(pcszLoggerName, sulog::SuLogLevel::FATAL, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define SULOG_DEBUG(...) ;
#define SULOG_INFO(...) ;
#define SULOG_WARN(...) ;
#define SULOG_ERROR(...) ;
#define SULOG_FATAL(...) ;

#define SULOG_iDEBUG(...) ;
#define SULOG_iINFO(...) ;
#define SULOG_iWARN(...) ;
#define SULOG_iERROR(...) ;
#define SULOG_iFATAL(...) ;
#endif

namespace sulog
{
	enum SuLogLevel
	{
		DEBUG,
		INFO,
		WARN,
		ERR,
		FATAL
	};

	class LogMessage
	{
	public:
		// Empty message use to exit the thread
		LogMessage();
		LogMessage(
			SuLogLevel level, const char* pcszLogFile,
			const char* pcszLogFunc, long lLine,
			const char* pcszLogMsg
		);
		bool MarkOutput(SuLogLevel sllCurrentLevel);
		std::string ToString();
	private:
		SuLogLevel m_sllMsgLevel;
		SYSTEMTIME m_stLogTime;
		std::string m_strLogFilePath;
		std::string m_strLogFunc;
		long m_lLogLine;
		std::string m_strLogMsg;
	};

	class LogHandler
	{
	public:
		friend class Logger;
		LogHandler(SuLogLevel sllOutputLevel);
		LogHandler(const LogHandler &l) = delete;
		LogHandler(LogHandler &&r) = delete;
		virtual void WriteLog(LogMessage &msg) = 0;
	protected:
		SuLogLevel m_sllLevel;
	};

	class ConsoleLogHandler : public LogHandler
	{
	public:
		ConsoleLogHandler(SuLogLevel sllOutputLevel);
		virtual void WriteLog(LogMessage &msg);
	};

	class FileLogHandler : public LogHandler
	{
	public:
		FileLogHandler(const char* pcszLogFile, SuLogLevel sllOutputLevel);
		FileLogHandler(const FileLogHandler &l) = delete;
		FileLogHandler(FileLogHandler &&r) = delete;
		~FileLogHandler();
		virtual void WriteLog(LogMessage &msg);
	protected:
		const char* m_pcszLogFile;
		FILE *m_fp;
	};

	class DailyFileLogHandler :public FileLogHandler
	{
	public:
		DailyFileLogHandler(
			const char* pcszLogFile, SuLogLevel sllOutputLevel);
		virtual void WriteLog(LogMessage &msg);
	private:
		void __RotateCheckProcedure();
	};

	class Logger
	{
	public:
		Logger(std::string strName);
		Logger(const Logger &l) = delete;
		Logger(Logger &&r) = delete;
		~Logger();
		void AddHandler(LogHandler *pLogHdl);
		void PushLog(LogMessage &msg);
		void PushLog(
			SuLogLevel sllLevel, const char* pcszFile,
			const char* pcszFunc, long lLine,
			const char* pcszMsgContent
		);
		void MarkExit();
		std::string Name();

		// If the message pump queue is empty, return true
		// This function use to indicate the Logger has written
		// all the log message
		bool Idle();
		static bool cls_pub_async;
	private:
		static void __WriteLogThreadFunc(Logger *pself);
		void __Push(LogMessage &msg);
		void __Push(
			SuLogLevel sllLevel, const char* pcszFile,
			const char* pcszFunc, long lLine,
			const char* pcszMsgContent
		);
		LogMessage _Pop();

		std::string m_strName;

		HANDLE m_hSem;

		std::mutex m_mtxMsgPump;
		std::queue<LogMessage> m_qMsgPump;
		bool m_bExit;

		std::vector<LogHandler*> m_vecHdl;

		// Use InterlockedExchangeAdd to modify
		static LONG m_lLoggerCount;
	};

	class SuLog
	{
	public:
		static SuLog* GetLogMgr();
		SuLog(const SuLog &l) = delete;
		SuLog(SuLog &&r) = delete;
		~SuLog();
		void SetAsyncLog(bool bAsyncLogging);
		void AddLogger(const std::string &strLoggerName, LogHandler *pHdl);
		void FlushAllLogger();
		void TraceLog(
			const std::string &strLoggerName, SuLogLevel sllLevel,
			const char* pcszFile, const char* pcszFunc,
			long lLine, const char* pcszMsgContent
		);
	private:
		SuLog();
		std::vector<Logger*> m_vecLogger;
	};

	void AppendConsoleLog(SuLogLevel sllLevel);
	void AppendConsoleLog(const char* pcszLoggerName, SuLogLevel sllLevel);

	void AppendFileLog(const char* pcszFileLog, SuLogLevel sllLevel, bool bDaily);
	void AppendFileLog(const char* pcszLoggerName, const char* pcszFileLog, SuLogLevel sllLevel, bool bDaily);

	void TraceLog(const char* pcszLoggerName, SuLogLevel sllLevel, const char* pcszFile, const char* pcszFunc, long lLine, const char* format, ...);

	class LogGuard
	{
	public:
		LogGuard(bool bAsyncLogging);
		LogGuard(const LogGuard &l) = delete;
		LogGuard(LogGuard &&r) = delete;
		~LogGuard();
	private:
		void __FlushAllLogger();
	};
}

#endif // !_SULOG_H_
