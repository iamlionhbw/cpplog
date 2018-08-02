#ifndef _SULOG_H_
#define _SULOG_H_

#pragma once

#include <queue>
#include <vector>
#include <mutex>
#include <string>
#include <map>

#include <Windows.h>

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

class LogConfig
{
protected:
	LogConfig(SuLogLevel sllOutputLevel);
	LogConfig(const LogConfig &l) = delete;
	LogConfig(LogConfig &&r) = delete;
	friend class Logger;
	virtual void WriteLog(LogMessage &msg) = 0;
protected:
	SuLogLevel m_sllLevel;
};

class ConsoleLogConfig : public LogConfig
{
public:
	ConsoleLogConfig(SuLogLevel sllOutputLevel);
	virtual void WriteLog(LogMessage &msg);
};

class FileLogConfig : public LogConfig
{
public:
	FileLogConfig(const char* pcszLogFile, SuLogLevel sllOutputLevel);
	virtual void WriteLog(LogMessage &msg);
protected:
	const char* m_pcszLogFile;
	FILE *m_fp;
};

class DayRotateFileLogConfig :public FileLogConfig
{
public:
	DayRotateFileLogConfig(
		const char* pcszLogFile, SuLogLevel sllOutputLevel,
		int iPeriod, int iMaxKeepBackup
	);
	virtual void WriteLog(LogMessage msg);
};

class Logger
{
public:
	Logger(std::string strName);
	Logger(const Logger &l) = delete;
	Logger(Logger &&r) = delete;
	~Logger();
	void AddConfig(LogConfig *pLogConf);
	void PushLog(LogMessage &msg);
	void PushLog(
		SuLogLevel sllLevel, const char* pcszFile,
		const char* pcszFunc, long lLine,
		const char* pcszMsgContent
	);
	void MarkExit();
	std::string Name();
private:
	static void _WriteLogThreadFunc(Logger *pself);
	void _Push(LogMessage &msg);
	void _Push(
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

	std::vector<LogConfig*> m_vecHdl;

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
	void AddRootConfig();
	void AddLogger(const std::string &strLoggerName, LogConfig *plc);
	void TraceLog(
		const std::string &strLoggerName, SuLogLevel sllLevel, 
		const char* pcszFile, const char* pcszFunc, 
		long lLine, const char* pcszMsgContent
	);
private:
	SuLog();
	std::vector<Logger*> m_vecLogger;
};

#endif // !_SULOG_H_
