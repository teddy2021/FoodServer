
#include <boost/asio.hpp>
#include <exception>
#include <string>
#include <stdexcept>

using std::string;

#include "NetMessenger.hpp"
#include "UDPCommunicator.hpp"
#include "TCPCommunicator.hpp"
#include "Logger.hpp"

NetMessenger::NetMessenger(std::shared_ptr<boost::asio::io_context> io_context, protocol_type type):context(io_context), inbox(), outbox(){
	Logger::GetInstance().log("[NetMessenger::NetMessenger] shared io_context with protocol", debug_level::INFO);
	switch(type){
		case tcp:
			comms = std::make_shared<TCPCommunicator>(context, 0xDEAD);
			break;
		case udp:
			comms = std::make_shared<UDPCommunicator>(context, static_cast<unsigned short>(0xDEAD));
			break;
		default:
			return;
	}
}

NetMessenger::NetMessenger(protocol_type type):context(std::make_shared<boost::asio::io_context>()), inbox(), outbox(){
	Logger::GetInstance().log("[NetMessenger::NetMessenger] protocol type: " + std::to_string(static_cast<int>(type)), debug_level::INFO);
	switch(type){
		case tcp:
			comms = std::make_shared<TCPCommunicator>(context, 0xDEAD);
			break;
		case udp:
			comms = std::make_shared<UDPCommunicator>(context, static_cast<unsigned short>(0xDEAD));
			break;
		default:
			return;
	}
}

NetMessenger::NetMessenger(protocol_type type, unsigned short port):context(std::make_shared<boost::asio::io_context>()), inbox(), outbox(){
	Logger::GetInstance().log("[NetMessenger::NetMessenger] port: " + std::to_string(port), debug_level::INFO);
	switch(type){
		case tcp:
			comms = std::make_shared<TCPCommunicator>(context, port);
			break;
		case udp:
			comms = std::make_shared<UDPCommunicator>(context, port);
			break;
		default:
			return;
	}
}

NetMessenger::NetMessenger(protocol_type type, string address, unsigned short port):context(std::make_shared<boost::asio::io_context>()), inbox(), outbox(){
	Logger::GetInstance().log("[NetMessenger::NetMessenger] address: " + address + " port: " + std::to_string(port), debug_level::INFO);
	switch(type){
		case tcp:
			comms = std::make_shared<TCPCommunicator>(context, address, port);
			break;
		case udp:
			comms = std::make_shared<UDPCommunicator>(context, address, port);
			break;
		default:
			return;
	}
}

NetMessenger::NetMessenger(NetMessenger & other): comms(std::move(other.comms)),  context(other.context){
	Logger::GetInstance().log("[NetMessenger::NetMessenger] copy constructed", debug_level::DEBUG);
	inbox = other.inbox;
	outbox = other.outbox;
	toGoOut = other.toGoOut;
	other.comms = nullptr;
}

NetMessenger & NetMessenger::operator=(NetMessenger &other){
	NetMessenger temp(other);
	swap(*this, temp);
	return *this;
} 

NetMessenger & NetMessenger::operator=(NetMessenger &&other){
	NetMessenger temp(other);
	swap(*this, other);
	return *this;
}

NetMessenger::~NetMessenger(){
	comms = nullptr;
}

void NetMessenger::Receive(){
	Logger::GetInstance().log("[NetMessenger::Receive] receiving message", debug_level::DEBUG);
	int buffer = inbox.GetFreeBuffer();
	try{
		comms->Receive();
	}
	catch(std::runtime_error e){
		context->stop();
		throw std::runtime_error("[NetMessenger::Receive 1]\n\t" + string(e.what()));
	}
	if(comms->GetProtocol() == udp){
		string m_size = comms->GetMessage();
		int size = 0;
		try{  
			size = std::stoi(m_size);
		} 
		catch (std::invalid_argument e){
			throw std::runtime_error("[NetMessenger::Receive 1] the value of '" + m_size + "' is not a valid size.");
		}
		catch (std::runtime_error e){
			throw std::runtime_error("[NetMessenger::Receive 1] an error ocurred when reading the message size.\n\t" + string(e.what()));
		}
		catch (std::out_of_range e){
			throw std::runtime_error("[NetMessenger::Receive 1] a size of '" + m_size + "' is way too big.");
		}
		if(size < 0 || size > comms->maxSize()){
			throw std::runtime_error("[NetMessenger::Receive 1] packet received larger than allowed.");
		}
		comms->ResizeBuffer(size);
		try{
			comms->Receive();
		}
catch(std::runtime_error e){
			context->stop();
			throw std::runtime_error("[NetMessenger::Receive 2] an error ocurred when reading from the message\n\t" + string(e.what()));
		}
	}
	inbox.StoreMessage(buffer, comms->GetMessage());
	comms->ResetBuffer();
}

string NetMessenger::GetMessage(int buffer){
	return inbox.GetMessageFrom(buffer);
}

string NetMessenger::GetFirstMessage(){
	return inbox.Pop();
}

void NetMessenger::Connect(boost::asio::io_context& ctx, std::string address, unsigned short port){
	Logger::GetInstance().log("[NetMessenger::Connect] connecting to: " + address + ":" + std::to_string(port), debug_level::INFO);
	comms->Connect(ctx, address, port);
}

void NetMessenger::Send(string message){
	Logger::GetInstance().log("[NetMessenger::Send] sending message", debug_level::INFO);
	if(comms->GetProtocol() == udp){  
		int idx = outbox.GetFreeBuffer(); 
		outbox.StoreMessage(idx, std::to_string(message.size()));
		toGoOut += 1;
	}
	int idx = outbox.GetFreeBuffer();

	outbox.StoreMessage(idx, message);
	toGoOut += 1;
	SendNext();
}

void NetMessenger::SendTo(string message, Recipient recipient){
	Logger::GetInstance().log("[NetMessenger::SendTo] sending to recipient", debug_level::INFO);
	string cur_addr = "";
	try{
		cur_addr = comms->remote_address();
	}
	catch(std::exception e){}
	unsigned short cur_prt = 0;
	try{
		cur_prt = comms->remote_port();
	}
	catch(std::exception e){}
	comms->Connect(*context, recipient->address, recipient->port);
	Send(message);
}

void NetMessenger::SendNext(){
	Logger::GetInstance().log("[NetMessenger::SendNext] sending next message", debug_level::DEBUG);
	while(nullptr == GetRemoteEndpoint()){
		sleep(1);
	}
	if(toGoOut > 0){
		try{
			string message = outbox.Pop();
			comms->Send(message);
			toGoOut -= 1;
			SendNext();
		}
catch(std::runtime_error e){
		context->stop();
		throw std::runtime_error("[NetMessenger::SendNext()]\n\t" + string(e.what()));
	}
	}
}

Recipient NetMessenger::GetRemoteEndpoint(){
	Logger::GetInstance().log("[NetMessenger::GetRemoteEndpoint] getting remote endpoint", debug_level::DEBUG);
	string addr = comms->remote_address();
	unsigned short p = comms->remote_port();
	recipient rr;
	Recipient r = std::make_shared<recipient>(rr);
	r->address = addr;
	r->port = p;
	return r;
}

void NetMessenger::ReplyTo(string message, Recipient recipient){
	Logger::GetInstance().log("[NetMessenger::ReplyTo] replying to recipient", debug_level::INFO);
	comms->Connect(*context, recipient->address, recipient->port);
	comms->Reply(message);
}

