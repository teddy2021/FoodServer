#include "TCPCommunicator.hpp"
#include <iostream>

int main() {
    try {
        auto io_context = std::make_shared<boost::asio::io_context>();
        auto server = std::make_shared<TCPCommunicator>(io_context, 8080);
        
        std::cout << "Calling Accept..." << std::endl;
        server->Accept();
        std::cout << "Accept completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}