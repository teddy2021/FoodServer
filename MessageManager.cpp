#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include <iostream>

#include "Enums.hpp"

using std::string;

#include "MessageManager.hpp"

MessageManager::~MessageManager(){
	if(buffer.size() > 0){
		buffer.clear();
		status.clear();
	}
}

string MessageManager::Pop(){
	string msg("");
	int i = 0;
	while(msg.empty()){
		try{
			msg = GetMessageFrom(i);
		}
		catch(invalid_state_exception e){
			i += 1;
		}
		catch(std::runtime_error e){
			break;
		}
	}
	return msg;
}

string MessageManager::GetMessageFrom(int index){
	if(index >= status.size() || index < 0){
		throw std::runtime_error("[MessageManager::GetMessageFrom(" + std::to_string(index) + ")] out of bounds request for buffer of size " + std::to_string(status.size()));
	}
	if(status.empty() || status[index] != storing ){
		throw invalid_state_exception("Message requested from a non-storing buffer");
	}
	status[index] = dirty;
	return *buffer[index];
}

int MessageManager::GetFreeBuffer(){
	for(int i = 0; i < buffer.size(); i += 1){
		switch (status[i]){
			case dirty:
				status[i] = in_use;
				*buffer[i] = "";
				return i;
			case clean:
				status[i] = in_use;
				return i;
			default:
				break;
		}
	}
	buffer.push_back(std::make_shared<string>(""));
	status.push_back(in_use);
	return buffer.size() - 1;
}

void MessageManager::StoreMessage(int index, string message){
	if(status.size() <= index || index < 0){
		throw std::runtime_error("[StoreMessage] Cannot store message at index " + std::to_string(index) + " for buffer of size " + std::to_string(status.size()));
	}
	if(status.empty() || status[index] != in_use ){
		throw invalid_state_exception("Attempting to store into a buffer that is not reserved");
	}
	*buffer[index] = string(message);
	status[index] = storing;
}

