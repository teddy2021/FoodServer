#pragma once
#include <fstream>
#include <string>
enum class debug_level { INFO=0, DEBUG=1,  WARN=2, ERROR=3};
class Logger{

	private:
		std::string logfile;
		debug_level detail;
		std::ofstream outstream;

		
		
		Logger(std::string path="log.log", debug_level dl=debug_level::INFO);
	public:


		~Logger();
		static Logger& GetInstance(){
			static Logger instance;
			return instance;
		};

		void SetLevel(debug_level lvl);
		void log(std::string msg, debug_level level);
};
