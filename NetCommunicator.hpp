
#pragma once
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ts/internet.hpp>
#include <boost/make_shared.hpp>
#include <memory>

#include "Enums.hpp"

class Communicator{

	protected:
		boost::system::error_code error;

		std::unique_ptr<std::string> recv_buffer;
		boost::shared_ptr<std::string> outgoing;
		bool connected = false;

		virtual void StoreMessage(const boost::system::error_code &err,
				std::size_t transferred){};
		virtual void HandleSend(boost::shared_ptr<std::string> message,
				const boost::system::error_code &err, 
				std::size_t transferred){};

	public:
		virtual void ResizeOutgoingBuffer(unsigned int size){};
		virtual void ResizeBuffer(unsigned int size){};
		virtual void ResetBuffer(){};
		virtual std::string GetMessage(){return "";};

		virtual void Send(std::string message){};
		
		virtual void Receive(){};
		virtual std::string remote_address(){return "";};
		virtual unsigned short remote_port(){return 0;};

		virtual void Connect(boost::asio::io_context & context, std::string address){};
		virtual void Connect(boost::asio::io_context & context, std::string address, unsigned int port){};

		virtual protocol_type GetProtocol(){return udp;};
		virtual void Reply(std::string message){};

};

