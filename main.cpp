
#include <locale>
#include <iostream>
#include "Server.hpp"

int main(){
	try{
		std::locale::global(std::locale(""));
	}
	catch(const std::exception& e){
		std::cerr << "Warning: Failed to set locale: " << e.what() << std::endl;
	}
	Server server;
	server.Listen();
}
