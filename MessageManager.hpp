#pragma once
#include <vector>
#include <memory>

#include "Enums.hpp"


class MessageManager{

	private:
		std::vector<std::shared_ptr<std::string>> buffer;
		std::vector<buffer_state> status;

	public:
		MessageManager(): buffer(), status(){};
		~MessageManager();

		std::string GetMessageFrom(int index);
		int GetFreeBuffer();
		
		std::string Pop();
		
		void StoreMessage(int index, std::string message);
};

