#pragma once
#include "NetCommunicator.hpp"
class UDPCommunicator : public Communicator{

	private:
		boost::asio::ip::udp::socket socket;
		boost::asio::ip::udp::endpoint remote_end;
		unsigned short prt;
		std::string addr;
		unsigned int msgSize;


		int Prime();

		void HandleSend(boost::shared_ptr<std::string> message,
				const boost::system::error_code &err, 
				std::size_t transferred) override;
		
		void StoreMessage(const boost::system::error_code &err,
				std::size_t transferred) override;
		void ResizeBuffer(unsigned int size) override;
		void ResizeOutgoingBuffer(unsigned int size) override;
		void ResetBuffer() override;
		
	public:

		std::string GetMessage() override;

		void Connect(boost::asio::io_context & context, std::string address) override;
		void Connect(boost::asio::io_context & context, std::string address, unsigned int port) override;
		UDPCommunicator() = delete;
		UDPCommunicator(boost::asio::io_context & context, unsigned int maxMessage=508):
			socket(context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0xBEEF)),
			addr("0.0.0.0"),
			prt(0xBEEF),
			msgSize(maxMessage){
				// TODO: CRITICAL - Buffer overflow risk: Fixed buffer size (10 bytes) can cause overflow
				recv_buffer = std::make_unique<std::string>(std::string("0", 10));
				recv_buffer->reserve(10);
				// TODO: Add bounds checking for all buffer operations to prevent overflow
				outgoing = boost::make_shared<std::string>(std::string("",1024));
		};

		UDPCommunicator(boost::asio::io_context & context, unsigned short port, unsigned int maxMessage=508):
			addr("0.0.0.0"),
			prt(port),
			socket(context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port)),
			msgSize(maxMessage){
				// TODO: CRITICAL - Buffer overflow risk: Fixed 10-byte buffer is too small for UDP messages
				recv_buffer = std::make_unique<std::string>(std::string("0", 10));
				recv_buffer->reserve(10);
				// TODO: Implement dynamic buffer sizing based on actual message length
				outgoing = boost::make_shared<std::string>(std::string("",1024));
		};
		
		UDPCommunicator(boost::asio::io_context & context, std::string address, unsigned short port, unsigned int maxMessage=508):
			addr(address),
			prt(port),
			socket(context, boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(address), port)), msgSize(maxMessage){
				// TODO: CRITICAL - Same buffer overflow risk as other constructors
				recv_buffer = std::make_unique<std::string>(std::string("0", 10));
				recv_buffer->reserve(10);
				// TODO: Add message size validation before buffer operations
				outgoing = boost::make_shared<std::string>(std::string("",1024));
		};

		UDPCommunicator(UDPCommunicator &other): socket(std::move(other.socket)){
			addr = other.addr;
			prt = other.prt;
			recv_buffer = std::move(other.recv_buffer);
			outgoing = std::move(other.outgoing);
			msgSize = other.msgSize;
		};
	

		UDPCommunicator & operator=(UDPCommunicator & other){
			UDPCommunicator temp(other);
			swap(*this, other);
			return *this;
		};


		UDPCommunicator & operator=(UDPCommunicator &&other){
			UDPCommunicator temp(other);
			swap(*this, other);
			return *this;
		};

		~UDPCommunicator();


		friend void swap(UDPCommunicator first, UDPCommunicator second){
			std::swap(first.addr, second.addr);
			std::swap(first.prt, second.prt);
			std::swap(first.socket, second.socket);
			std::swap(first.remote_end, second.remote_end);
			std::swap(first.recv_buffer, second.recv_buffer);
			std::swap(first.outgoing, second.outgoing);
			std::swap(first.msgSize, second.msgSize);
		}

		void Send(std::string message) override;
		void Reply(std::string message) override;

		void Receive() override;


		std::string remote_address() override;
		unsigned short remote_port() override;


		protocol_type GetProtocol() override;
		unsigned int maxSize() override;
};

