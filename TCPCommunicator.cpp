

#include <boost/asio/basic_socket.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/error.hpp>
#include <catch2/catch_tostring.hpp>
#include <stdexcept>
#include <string>
#include <iostream>

#include <boost/make_shared.hpp>
#include <boost/bind/bind.hpp>

#include "TCPCommunicator.hpp"

using std::string;
using boost::shared_ptr;
using boost::asio::buffer;
using std::cerr;
using boost::asio::ip::tcp;

TCPCommunicator::~TCPCommunicator(){
	if(socket.is_open()){  
		socket.close();
	}
	if(acceptor.is_open()){  
		acceptor.close();
	}
	if(recv_buffer != nullptr &&recv_buffer->size() > 0){  
		recv_buffer->erase();
	}
	if(outgoing != nullptr && outgoing->size() > 0){  	
		outgoing->erase();
	}
}

void TCPCommunicator::HandleSend(shared_ptr<string> message,
		const boost::system::error_code &err,
		std::size_t transferred){
	if(err){
		cerr << "[TCPCommunicator::HandleSend] An error occurred:\n\t" << err.message() << "\n";
		string first_half = message->substr(0, (message->size() / 2) - 1);
		string second_half = message->substr((message->size() / 2), message->size() - 1);
		Send(first_half);
		Send(second_half);
		throw std::runtime_error("[TCPCommunicator::HandleSend(" + *message + ",__, ___)] Failed to send message:\n\t" + err.message());
	}
	else{
		connected = true;
		outgoing->clear();
	}
}


void TCPCommunicator::Send(string message){
	if(!connected){
		throw std::runtime_error( "[TCPCommunicator::Send(" + message + ")]: Not connected to a peer.\n");
	}
	outgoing->resize(message.size() + 2);
	*outgoing = message;
	socket.async_send(buffer(*outgoing),
			boost::bind(&TCPCommunicator::HandleSend,
				this,
				outgoing,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
}


void TCPCommunicator::Reply(string message){
	if(!connected){
		throw std::runtime_error("[TCPCommunicator::Reply(" + message + ")] Not connected to a peer.");
	}
	outgoing->clear();
	outgoing->resize(message.size());
	*outgoing = message;
	boost::asio::async_write(socket, 
			buffer(*outgoing),	
			boost::bind(&TCPCommunicator::HandleSend, 
				this,
				outgoing,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));

}


void TCPCommunicator::HandleAccept(boost::system::error_code err){
	if(err){
		cerr << "[TCPCommunicator::HandleAccept] An error occurred:\n\t" << err.message() << "\n";
	}
	else{
		connected = true;
	}
}


void TCPCommunicator::Accept(){
	if(!acceptor.is_open()){
		acceptor.open(tcp::v4());
	}
	socket = tcp::socket(socket.get_executor());
	acceptor.async_accept(socket,
		   	boost::bind(
				&TCPCommunicator::HandleAccept,
			   	this, 
				boost::asio::placeholders::error));
//	res = acceptor.accept(socket, error);
//	if(!error && !res){
//		remote_end = socket.remote_endpoint();
//		std::cout << "[TCPCommunicator::Accept()] connected to " << remote_end.address().to_string() << ":" << remote_end.port() << "\n";
//		connected = true;
//	}
//	else{
//		cerr << "[TCPCommunicator::Accept] Failed to connect.\n\t" << error.message() << "\n"; 
//	}
}


void TCPCommunicator::ResizeBuffer(unsigned int size){
	recv_buffer->resize(size);
	recv_buffer->shrink_to_fit();
}


void TCPCommunicator::ResetBuffer(){
	recv_buffer->erase();
}


void TCPCommunicator::StoreMessage(const boost::system::error_code &err,
		std::size_t transferred){
	if(!err){
		ResizeBuffer(transferred);
	}
	if(err){
		throw std::runtime_error("[TCPCommunicator::StoreMessage] an error occurred while recieving the message.\n\t" + err.message());
	}
	if(socket.available() > 0){
		Receive();
	}
}


void TCPCommunicator::Receive(){
	if(!connected){
		Accept();
	}
	socket.async_read_some(buffer(*recv_buffer),
			boost::bind(&TCPCommunicator::StoreMessage,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
}


string TCPCommunicator::GetMessage(){
	string out(*recv_buffer);
	ResetBuffer();
	return out;
}

string TCPCommunicator::remote_address(){
	return socket.remote_endpoint().address().to_string();
}


unsigned short TCPCommunicator::remote_port(){
	return static_cast<unsigned short>(socket.remote_endpoint().port());
}


protocol_type TCPCommunicator::GetProtocol(){
	return tcp;
}


void TCPCommunicator::HandleConnect(boost::system::error_code err, 
		tcp::endpoint ep){
	if(err){
		cerr << "[TCPCommunicator::HandleConnect] An error occurred.\n\t" << err.message() << "\n";
	}else{
		connected = true;
	}
}


void TCPCommunicator::Connect(boost::asio::io_context & context, string address){
	tcp::resolver resolver(context);
	tcp::resolver::results_type res = resolver.resolve(address, std::to_string(0xBEEF));
	if(res.empty()){
		throw std::runtime_error("Failed to resolve any peers on " + address + ":0xBEEF");
	}
	if(socket.is_open()){
		socket.close();
	}
	boost::asio::async_connect(socket, res, boost::bind(
				&TCPCommunicator::HandleConnect,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::endpoint
				));
//	for(auto it = res.begin(); it != res.end(); it ++){
//		if(it->endpoint().address().is_v4()){
//			socket.connect(it->endpoint());
//		}
//	}
}


void TCPCommunicator::Connect(boost::asio::io_context &context, std::string address, unsigned int port){
	tcp::resolver resolver(context);
	tcp::resolver::results_type res = resolver.resolve(address, std::to_string(port));
	if(res.empty()){
		throw std::runtime_error("Failed to resolve any peers on " + address + ":" + std::to_string(port));
	}
	
	boost::asio::async_connect(socket, res, boost::bind(
						&TCPCommunicator::HandleConnect,
						this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::endpoint
						));

//	for(auto it = res.begin(); it != res.end(); it ++){
//		if(it->endpoint().address().is_v4() && it->endpoint().port() == port){
//			boost::system::error_code err;
//			std::cout << "Trying to connect to " << it->endpoint().address().to_string() << ":" << it->endpoint().port() << "\n";
//			socket.connect(it->endpoint(), err);
//			if(err){
//				std::cout << "\tFailure. " << err.message() << "\n";
//			}
//			else{
//				std::cout << "\tSuccess.\n";
//				break;
//			}
//			
//		}
//	}
}




