
#include <algorithm>
#include <boost/filesystem/file_status.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <exception>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>
#include <boost/filesystem.hpp>
using std::string;
using std::ifstream;
using fs_path = boost::filesystem::path;

#include "FileManager.hpp"

bool is_prefix(fs_path p1, boost::filesystem::path p2){
	bool res = false;

	fs_path copy_1(p1);
	fs_path copy_2(p2);
	if(copy_1.filename() == "."){
		copy_1.remove_filename();
	}
	assert(copy_2.has_filename());
	copy_2.remove_filename();

	auto copy_1_len = std::distance(copy_1.begin(), copy_1.end());
	auto copy_2_len = std::distance(copy_2.begin(), copy_2.end());
	if(copy_1_len > copy_2_len){
		return false;
	}
	return std::equal(copy_1.begin(), copy_1.end(), copy_2.begin());
}

FileManager::FileManager(string path, size_t lineLen): filepath(path){
	if(path.empty()){
		return;
	}
	ifstream file;
	
	fs_path FS_path(path);
	if(!(boost::filesystem::exists(FS_path))){
		throw std::runtime_error("[FileManager(" + path + ")] File does not exist");
	}

	try{
		fs_path can = boost::filesystem::canonical(FS_path);
		if(!is_prefix(boost::filesystem::current_path(), can)){
			throw std::runtime_error("[FileManager(" + path +")] File is not under the hierarchy of the current directory");
		}
	}
	catch(std::exception e){
		throw std::runtime_error("[FileManager(" + path + ")] Failed to get cannonical path.");
	}
		
	file.open(FS_path.string());
	if(!file.is_open()){
		throw std::runtime_error("[FileManager(" + path + ")] File is not opened.");
	}
	file.seekg(0, file.beg);

	string line;
	while(std::getline(file, line)){
		if(line.size() > lineLen){
			size_t beg = 0;
			size_t end = lineLen - 1;
			while(end < line.size()){
				string sub = line.substr(beg, end);
				data.push_back(sub);
				beg = end + 1;
				end = beg + std::min(lineLen, line.size() - beg);
			}
		}

		data.push_back(string(line));
	}

	file.close();
}

int FileManager::getLineCount(){
	return data.size();
}

string FileManager::getLine(int lineIDX){
	if(lineIDX >= data.size() || lineIDX < 0){
		throw std::out_of_range("FileManager(" + std::to_string(lineIDX) + "): index less than 0 or greater than " + std::to_string(data.size()));
	}
	return data[lineIDX];
}
