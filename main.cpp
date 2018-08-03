/* A simple log tools for CPP program
 * 
 * Notice:
 * SuLog use the thread object which is part of C++11 standard
 * library to implement asynchronous log I/O
 * If fact, you can change it to synchronous mode. See the comment.
 */
#include "sulog.h"

int main()
{
	// sulog is namespace.

	// First, init a LogGuard object.
	// SuLog will work on asynchronous mode if the parameter
	// passed to the LogGuard constructor is true, while false
	// for synchronous mode.
	// In the asynchronous mode, all the message content would
	// be pushed to a queue then be popped to do the actual output in
	// the background thread. It is conspicuous when use multiple
	// logger. You can see the output order in the following example.
	// In the synchronous mode, all the message would be write through
	// to the output channel where they are coded.
	// To see the difference, you can try to modify the bool variable.
	// Since we init the LogGuard here,
	// you should be awared of that it is !SO DANGEROUS! to 
	// use sulog inside any global object constructor.
	sulog::LogGuard lg(true);

	// Next step, in fact, there is a logger called _ROOT_ created
	// by the Log Manager automatically. But this _ROOT_ does nothing.
	// So in order to perform a basic console output,
	// we have to add a output handler to the _ROOT_
	sulog::AppendConsoleLog(sulog::SuLogLevel::DEBUG);

	// Sometimes, there will be many teammates participated in
	// a same project or solution. At this moment, you can
	// add another logger to seperate different log message.
	// It could be very useful especially in file log.
	// eg. You can add a normal file log to write the main message,
	//     meanwhile, your teammates can write other message for their
	//     own module which may cause mass output strings.

	// If you need to output message to a file
	// Parameter 1 is the Logger name, it should be unique global.
	// Parameter 2 is the file log path, keep in your mind, it
	//             doesn't check whether the directory exists
	//             or not, so you have to confirm this. It is
	//             undefined behavior if the directory doesn't exist.
	// Parameter 3 if true, then it will create a daily log file
	sulog::AppendFileLog("MateB", "./teammateB.log", sulog::SuLogLevel::INFO, false);
	// After the above line, a Logger named "MateB" would be added
	// to the Log Manager. But, wait, the MateB logger just write
	// message to file, you still can not see anything from the
	// console. So just like what we done for the _ROOT_:
	sulog::AppendConsoleLog("MateB", sulog::SuLogLevel::DEBUG);
	// Notice: if a Named Logger is already in the Log Manager,
	// it won't be constructed again.
	// Remember AppendFileLog and AppendConsoleLog is enough,
	// If you want to add file log for the _ROOT_, just call
	// AppendFileLog with two parameters, for example:
	// sulog::AppendFileLog("./main.log", sulog::SuLogLevel::DEUG, false);
	
	// SULOG_x is for root logger
	SULOG_DEBUG("%s:%d", "I am ", 18);
	SULOG_INFO("%s:%d", "I am ", 18);
	SULOG_WARN("%s:%d", "I am ", 18);
	SULOG_ERROR("%s:%d", "I am ", 18);
	SULOG_FATAL("%s:%d", "I am ", 18);

	// To use MateB logger, use the SULOG_ix instead.
	// Notice the level set by AppendFileLog above
	SULOG_iDEBUG("MateB", "Hello world");
	SULOG_iINFO("MateB", "Hello world");

	return 0;
}
