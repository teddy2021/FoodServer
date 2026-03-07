#pragma once
#include <fstream>
#include <string>
enum class debug_level { DEBUG, INFO, WARN, ERROR };
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
