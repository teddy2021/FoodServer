
#include <string>
#include <vector>


class FileManager{
	private:
		std::string filepath;
		std::vector<std::string> data;

	public:
		FileManager(std::string path);
		int getLineCount();
		std::string getLine(int lineIDX);
};
