#pragma once
#include <ios>
#include <iostream>
#include "NetCommunicator.hpp"
class UDPCommunicator : public Communicator{

	private:
		boost::asio::ip::udp::socket socket;
		boost::asio::ip::udp::endpoint remote_end;
		unsigned short prt;
		std::string addr;
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
		UDPCommunicator():
			addr("0.0.0.0"),
			prt(0xBEEF),
			socket(boost::asio::io_context().get_executor()){
				recv_buffer = std::make_unique<std::string>(std::string("0", 10));
				recv_buffer->reserve(10);
				outgoing = boost::make_shared<std::string>(std::string("",1024));
		};

		UDPCommunicator(boost::asio::io_context & context):
			socket(context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0xBEEF)),
			addr("0.0.0.0"),
			prt(0xBEEF){
				recv_buffer = std::make_unique<std::string>(std::string("0", 10));
				recv_buffer->reserve(10);
				outgoing = boost::make_shared<std::string>(std::string("",1024));
		};

		UDPCommunicator(boost::asio::io_context & context, unsigned short port):
			addr("0.0.0.0"),
			prt(port),
			socket(context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port)){
				recv_buffer = std::make_unique<std::string>(std::string("0", 10));
				recv_buffer->reserve(10);
				outgoing = boost::make_shared<std::string>(std::string("",1024));
		};
		
		UDPCommunicator(boost::asio::io_context & context, std::string address, unsigned short port):
			addr(address),
			prt(port),
			socket(context, boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(address), port)){
				recv_buffer = std::make_unique<std::string>(std::string("0", 10));
				recv_buffer->reserve(10);
				outgoing = boost::make_shared<std::string>(std::string("",1024));
		};

		UDPCommunicator(UDPCommunicator &other): socket(std::move(other.socket)){
			addr = other.addr;
			prt = other.prt;
			recv_buffer = std::move(other.recv_buffer);
			outgoing = std::move(other.outgoing);
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
		}

		void Send(std::string message) override;
		void Reply(std::string message) override;

		void Receive() override;


		std::string remote_address() override;
		unsigned short remote_port() override;


		protocol_type GetProtocol() override;

};

