#pragma once
#include "Banner.h"
#include <iostream>
#include <cstdarg>
#include <string>
#include <sstream>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <stdexcept>

#define COLOR_CLEAR  "\033[0m"
#define COLOR_BLACK  "\033[30m"
#define COLOR_RED    "\033[31m"
#define COLOR_GREEN  "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE   "\033[34m"
#define COLOR_MAGE   "\033[35m"
#define COLOR_CYAN   "\033[36m"


namespace anv_log
{
	// TODO: impl
	enum class LogLevel
	{
		LL_NONE,
		LL_INFO,
		LL_DEBUG,
		LL_WARN,
		LL_ERROR,
		LL_FATAL
	};

	enum class TermColor
	{
		TC_NONE,
		TC_BLACK,
		TC_RED,
		TC_GREEN,
		TC_YELLOW,
		TC_BLUE,
		TC_MAGENTA,
		TC_CYAN,
	};

	struct LogCreateInfo
	{
		std::string fileBanner  = ANV_STD_LOG_BANNER; // Set the banner at the beginning of the file
		std::string logFilePath = "AnvLog.log";       // Set the path of the log file
		std::string logFileName = "Logfile.alog";     // Set Name
		std::string timeFormat  = "%I:%M:%S";         // Set the time format
		bool		consoleOutput = true;             // Set if you want standard console output
		bool		fileOutput    = false;            // Set if you want file output
		bool        abortOnError   = false;           // Program abort if Error is logged
	};

	struct File;


	// a general pourpose logging class that 
	// can be used anywhere within the codebase.
	class AnvLog
	{
	public:
		static void Init(const LogCreateInfo _info);

		// Custom logging msg
		static void LOG_CUST(TermColor _col, LogLevel _lev, const std::string _str, ...);
		static void LOG_INFO(const std::string _str, ...);
		static void LOG_DEBUG(const std::string _str, ...);
		static void LOG_WARN(const std::string _str, ...);
		static void LOG_ERROR(const std::string _str, ...);
		static void LOG_FATAL(const std::string _fn, const std::string _str, ...);

		static std::string GetTime();

		static std::string formatString(const std::string& format, va_list args);
	private:
		static std::string level_to_string(const LogLevel _lev);
		static std::string color_to_string(const TermColor _col);

		static inline std::shared_ptr<File> m_File = nullptr;
		static inline LogCreateInfo m_CreationInfo = {};

		friend struct File;
	};
}