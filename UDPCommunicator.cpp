
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ts/internet.hpp>
#include <boost/bind/bind.hpp>
#include <boost/lexical_cast.hpp>

#include <stdexcept>
#include <iostream>

#include "UDPCommunicator.hpp"
#include "Enums.hpp"
using boost::asio::buffer;
using std::string;
using boost::shared_ptr;
using std::cerr;


UDPCommunicator::~UDPCommunicator(){
	if(socket.is_open()) {
		socket.close();
	}
	if(outgoing != nullptr && outgoing->size() > 0) {
	   	outgoing->erase(); 
	}
	if(recv_buffer != nullptr && recv_buffer->size() > 0) {
	   	recv_buffer->erase(); 
	}
}

string UDPCommunicator::GetMessage(){
	string out(*recv_buffer);
	return out;
}

void UDPCommunicator::ResizeBuffer(unsigned int size){
	if(recv_buffer->size() < size){
		recv_buffer->resize(size);
	}
	else{
		*recv_buffer = recv_buffer->substr(0, size);
	}
}

void UDPCommunicator::ResetBuffer(){
	recv_buffer->clear();
	ResizeBuffer(10);
}

void UDPCommunicator::ResizeOutgoingBuffer(unsigned int size){
	if(outgoing->size() < size){
		*outgoing = recv_buffer->substr(0,size);
	}
	else{
		outgoing->resize(size);
		outgoing->shrink_to_fit();
	}
}


void UDPCommunicator::HandleSend(boost::shared_ptr<std::string> message, 
		const boost::system::error_code &err,
		std::size_t transferred){
	if(err){
		string first_half = message->substr(0, (message->size() / 2) - 1);
		string second_half = message->substr((message->size() / 2), message->size() - 1);
		Send(first_half);
		Send(second_half);
		throw std::runtime_error("[UDPCommunicator::HandleSend(" + *message + ",__, ___)] Failed to send message:\n\t" + err.message());
	}
	else{
		connected = true;
	}
}

void UDPCommunicator::Send(string message){
	if(!connected){
		throw std::runtime_error("[UDPCommunicator::Send] Unable to send without an endpoint.");
	}
	ResizeOutgoingBuffer(message.size());
	*outgoing = message;
	socket.async_send_to(buffer(*outgoing),
			remote_end,
			boost::bind(&UDPCommunicator::HandleSend,
				this,
				outgoing,
				boost::asio::placeholders::error, 
				boost::asio::placeholders::bytes_transferred));

	//   	try{	
	//	} catch (boost::wrapexcept<boost::system::system_error>){
//			remote_end.port() << "\n";
//	}
	if(error){
		cerr << "[Send(" << message.substr(0,80) << ")] Failed to send message:\n\t" << error.message() << "\n";
		throw std::runtime_error("[UDPCommunicator::Send(" + message + ")] Failed to send message:\n\t" + error.message());
	}

}

void UDPCommunicator::Reply(string message){
	if(!connected){
		throw std::runtime_error("[UDPCommunicator::Reply]Cannot reply without first being contacted");
	}
	outgoing->resize(message.size());
	outgoing->shrink_to_fit();
	*outgoing = message;
	socket.async_send_to(buffer(*outgoing),
			remote_end,	
			boost::bind(&UDPCommunicator::HandleSend,
				this,
				outgoing,
				boost::asio::placeholders::error, 
				boost::asio::placeholders::bytes_transferred));

	//   	try{	
	//	} catch (boost::wrapexcept<boost::system::system_error>){
//			remote_end.port() << "\n";
//	}
	if(error){
		throw std::runtime_error("[UDPCommunicator::Reply(" + message.substr(0,80) + ")] Failed to send message:\n\t" + error.message());
	}
}


void UDPCommunicator::Receive(){
	if(!connected){
		connected = true;
	}
	socket.async_receive_from(buffer(*recv_buffer),
			remote_end,
			boost::bind(&UDPCommunicator::StoreMessage, 
				this, 
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
}

void UDPCommunicator::StoreMessage(const boost::system::error_code &err, 
		std::size_t transferred){
	if(!err){
		ResizeBuffer(transferred);
		connected = true;
	}
	if(err){
		throw std::runtime_error("[UDPCommunicator::StoreMessage( ___, " + std::to_string(transferred) + ")] An error ocurred.\n\t" + err.message());
	}
}



void UDPCommunicator::Connect(boost::asio::io_context & context, string address){
	boost::asio::ip::udp::resolver r(context);
	boost::asio::ip::udp::resolver::results_type res = r.resolve(address, std::to_string(0xBEEF));
	for(auto it = res.begin() ; it != res.end(); it ++){
		if(it->endpoint().address().is_v4()){
			remote_end = it->endpoint();
			break;
		}
	}
	connected = true;
}

void UDPCommunicator::Connect(boost::asio::io_context & context,string address, unsigned int port){
	boost::asio::ip::udp::resolver r(context);
	boost::asio::ip::udp::resolver::results_type res = r.resolve(address, std::to_string(port));
	for(auto it = res.begin() ; it != res.end(); it ++){
		if(it->endpoint().address().is_v4() && it->endpoint().port() == port){
			remote_end = it->endpoint();
			break;
		}
	}
	connected = true;
}


string UDPCommunicator::remote_address(){
	return remote_end.address().to_string();
}

unsigned short UDPCommunicator::remote_port(){
	return static_cast<unsigned short>(remote_end.port());
}

protocol_type UDPCommunicator::GetProtocol(){
	return udp;
}
