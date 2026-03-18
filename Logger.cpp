#include "Logger.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

using std::string;


Logger::Logger(string path, debug_level dl): detail(dl), logfile(path){
	outstream = std::ofstream(logfile);
}


void Logger::log(std::string msg, debug_level level){
	if(!outstream.is_open()){
		throw std::runtime_error("[Logger::log]: output file is not open");
	}
	auto now = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now);
	std::ostringstream oss;
	oss << std::put_time(std::localtime(&t), "%d/%m/%Y %H:%M:%S");
	string human_time = oss.str();
	string out;
	switch(level){
		case debug_level::ERROR:
			if(detail < debug_level::ERROR){
				return;
			}
			out = "[Error][" + human_time + "]: " + msg;
			break;
		case debug_level::WARN:
			if(detail < debug_level::WARN){
				return;
			}
			out = "[Warning][" + human_time + "]: " + msg;
			break;
		case debug_level::INFO:
			out = "[Information][" + human_time + "]: " + msg;
			break;
		case debug_level::DEBUG:
			if(detail < debug_level::DEBUG){
				return;
			}
			out = "[Debug][" + human_time + "]: " + msg;
			break;
		default:
			break;
	}
	out += "\n";
	outstream.write(out.c_str(), out.size());
	outstream.flush();
}


void Logger::SetLevel(debug_level lvl){
	detail = lvl;
}


Logger::~Logger(){
	if(outstream.is_open()){
		outstream.flush();
		outstream.close();
	}
}
