
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>

using std::string;
using std::ifstream;


#include "FileManager.hpp"

FileManager::FileManager(string path): filepath(path){
	if(path.empty()){
		return;
	}
	ifstream file;
	file.open(path);
	if(!file.is_open()){
		throw std::runtime_error("[FileManager(" + path + ")] File is not opened.");
	}
	file.seekg(0, file.beg);

	string line;
	while(std::getline(file, line)){
		data.push_back(string(line));
	}

	file.close();
}

int FileManager::getLineCount(){
	return data.size();
}

string FileManager::getLine(int lineIDX){
	return data[lineIDX];
}
