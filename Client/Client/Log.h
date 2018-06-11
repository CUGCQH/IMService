#pragma once
#ifdef WIN32    
#include <io.h>  
#include <direct.h>    
#endif    
#ifdef linux     
#include <unistd.h>   
#include <sys/types.h>    
#include <sys/stat.h>   
#endif
#include <ctime>
static bool check_dir(const char* dir)
{
	if (_access(dir, 0) == -1)
	{
#ifdef WIN32    
		int flag = _mkdir(dir);
#endif    
#ifdef linux     
		int flag = mkdir(dir.c_str(), 0777);
#endif    
		return (flag == 0);
	}
	return true;
};


#ifdef _WIN32  
#define __FILENAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1):__FILE__)  
#else  
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)  
#endif 

#ifndef suffix  
#define suffix(msg)  std::string(msg).append("  //")\
.append(__FILENAME__).append(":").append(__func__)\
.append("()#").append(std::to_string(__LINE__))\
.append(".").c_str()
#endif  


#include "../../spdlog/spdlog.h"

class Log
{
public:
	Log()
	{
		check_dir("logs");
		spdlog::set_async_mode(32768);

		nml_logger = spdlog::basic_logger_mt("basic_logger", "logs/log.txt");		
		nml_logger->set_pattern("%T.%e %t [%L] %v"); 
		nml_logger->flush_on(spdlog::level::trace);

		nml_logger_console = spdlog::stdout_color_mt("console");
		nml_logger_console->set_pattern("%T.%e %t [%L] %v");
	};
	~Log() 
	{
		spdlog::drop_all();
	};
	auto getLogger() { return nml_logger; }
	auto getConsoleLogger() { return nml_logger_console; }

	//目前只有一个 logger 需要增加交易 logger  
	std::shared_ptr<spdlog::logger> nml_logger;
	std::shared_ptr<spdlog::logger> nml_logger_console;
};

extern Log OutLog;
#define LOG_TRACE(msg,...) OutLog.getLogger()->trace(suffix(msg),__VA_ARGS__)  ; OutLog.getConsoleLogger()->trace(suffix(msg),__VA_ARGS__)  
#define LOG_DEBUG(msg,...) OutLog.getLogger()->debug(suffix(msg),__VA_ARGS__)   ; OutLog.getConsoleLogger()->debug(suffix(msg),__VA_ARGS__)
#define LOG_ERROR(...) OutLog.getLogger()->error(__VA_ARGS__)   ; OutLog.getConsoleLogger()->error(__VA_ARGS__)
#define LOG_WARM(...) OutLog.getLogger()->warn(__VA_ARGS__)   ; OutLog.getConsoleLogger()->warn(__VA_ARGS__)
#define LOG_INFO(...) OutLog.getLogger()->info(__VA_ARGS__)   ; OutLog.getConsoleLogger()->info(__VA_ARGS__) 
#define LOG_CRITICAL(...) OutLog.getLogger()->critical(__VA_ARGS__)   ; OutLog.getConsoleLogger()->critical(__VA_ARGS__)