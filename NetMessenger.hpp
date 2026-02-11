#pragma once
#include <boost/asio.hpp>
#include <memory>

#include "NetCommunicator.hpp"
#include "MessageManager.hpp"
#include "Enums.hpp"


struct recipient {
	recipient(): address(16, ' '), port(0xDEAD){};
	recipient(recipient & other): address(other.address), port(other.port){};
	recipient(std::string addr, unsigned short prt): address(addr), port(prt){};
	std::string address;
	unsigned short port;
};
typedef std::shared_ptr<recipient> Recipient;

class NetMessenger{
	private:
		boost::asio::io_context context;
		std::shared_ptr<Communicator> comms;
		MessageManager inbox; 
		MessageManager outbox;

		int toGoOut = 0;


		void RunContext();
		void SendNext();
		std::string GetMessage(int buffer);

	public:
		NetMessenger(){};
		NetMessenger(protocol_type type);
		NetMessenger(protocol_type type, unsigned short port);
		NetMessenger(protocol_type type, std::string address, unsigned short port);
		NetMessenger(NetMessenger & other);

		~NetMessenger();

		NetMessenger & operator=(NetMessenger &other);
		NetMessenger & operator=(NetMessenger &&other);	

		friend void swap(NetMessenger first, NetMessenger second){
			std::swap(first.comms,  second.comms);
			std::swap(first.inbox,  second.inbox);
			std::swap(first.outbox, second.outbox);
			std::swap(first.toGoOut, second.toGoOut);
		};


		void Receive();
		void Send(std::string message);
		void SendTo(std::string message, Recipient recipient);
		void ReplyTo(std::string message, Recipient recipient);

		std::string GetFirstMessage();
		Recipient GetRemoteEndpoint();

};

