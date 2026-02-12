

#include <boost/asio/basic_socket.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/error.hpp>
#include <boost/system/detail/error_code.hpp>
#include <catch2/catch_tostring.hpp>
#include <memory>
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

void TCPCommunicator::validateSocketState() const {
	if(!socket){
		throw std::runtime_error("[TCPCommunicator::validateSocketState] Socket is null");
	}
	if(!socket->is_open()){
		throw std::runtime_error("[TCPCommunicator::validateSocketState] Socket is not open");
	}
}

TCPCommunicator::~TCPCommunicator(){
	boost::system::error_code err;
	
	// Clean up socket first
	if(socket && socket->is_open()){  
		boost::system::error_code ec = socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, err);
		if(ec || err){
			cerr << "[~TCPCommunicator()]: Failed to shutdown the socket.\n\t" << err.message() << " | " << ec.message();
		}
		ec = socket->close(err);
		if(ec || err){
			cerr << "[~TCPCommunicator()]: Failed to close the socket.\n\t." << err.message() << " | " << ec.message();
		}
	}
	
	// Clean up acceptor
	if(acceptor && acceptor->is_open()){  
		acceptor->close(err);
	}
	
	// Clean up buffers
	if(recv_buffer){  
		recv_buffer->clear();
	}
	if(outgoing){  	
		outgoing->clear();
	}
}




void TCPCommunicator::Send(string message){
	validateSocketState();
	if(!connected){
		throw std::runtime_error( "[TCPCommunicator::Send(" + message + ")]: Not connected to a peer.\n");
	}
	boost::system::error_code err;
	outgoing->resize(message.size() + 2);
	*outgoing = message;
	size_t transferred = socket->send(buffer(*outgoing), 0, err);
	if(err){
		cerr << "[TCPCommunicator::Send] An error occurred:\n\t" << err.message() << "\n";
		throw std::runtime_error("[TCPCommunicator::Send(" + message + ")] Failed to send message:\n\t" + err.message());
	}
	else{
		outgoing->clear();
	}
}


void TCPCommunicator::Reply(string message){
	validateSocketState();
	if(!connected){
		throw std::runtime_error("[TCPCommunicator::Reply(" + message + ")] Not connected to a peer.");
	}
	boost::system::error_code err;
	outgoing->clear();
	outgoing->resize(message.size());
	*outgoing = message;
	size_t transferred = boost::asio::write(*socket, buffer(*outgoing), err);
	if(err){
		cerr << "[TCPCommunicator::Reply] An error occurred:\n\t" << err.message() << "\n";
		throw std::runtime_error("[TCPCommunicator::Reply(" + message + ")] Failed to send message:\n\t" + err.message());
	}
	else{
		outgoing->clear();
	}
}





void TCPCommunicator::Accept(){
	boost::system::error_code err;
	
	// Only accept if we're in server mode and have an acceptor
	if(!is_server_mode || !acceptor){
		throw std::runtime_error("[TCPCommunicator::Accept] Not configured as server");
	}
	
	// Ensure acceptor is ready
	if(!acceptor->is_open()){
		acceptor->open(tcp::v4(), err);
		if(err){
			throw std::runtime_error("[TCPCommunicator::Accept] Failed to open acceptor:\n\t" + err.message());
		}
		acceptor->listen();
		if(err){
			throw std::runtime_error("[TCPCommunicator::Accept] Failed to start listening:\n\t" + err.message());
		}
	}
	
	if(socket->is_open()){
		socket->close();
	}
	socket = std::make_unique<boost::asio::ip::tcp::socket>(*io_context_ref);
	// Accept the connection
	acceptor->accept(*socket, remote_end, err);
	if(err ){
		cerr << "[TCPCommunicator::Accept] An error occurred:\n\t" << err.message() << "\n";
		throw std::runtime_error("[TCPCommunicator::Accept] Failed to accept connection:\n\t" + err.message() + "\n");
	}
	else{
		connected = true;
	}
}


void TCPCommunicator::ResizeBuffer(unsigned int size){
	recv_buffer->resize(size);
	recv_buffer->shrink_to_fit();
}


void TCPCommunicator::ResetBuffer(){
	recv_buffer->erase();
}





void TCPCommunicator::Receive(){
	if(!connected){
		Accept();
	}
	if(!socket->is_open()){
		throw std::runtime_error("[TCPCommunicator::Receive()]: Socket is not open.");
	}
	boost::system::error_code err;
	size_t transferred = socket->read_some(buffer(*recv_buffer), err);
	if(err){
		throw std::runtime_error("[TCPCommunicator::Receive] an error occurred while receiving the message.\n\t" + err.message());
	}
	ResizeBuffer(transferred);
	if(socket->available() > 0){
		Receive();
	}
}


string TCPCommunicator::GetMessage(){
	string out(*recv_buffer);
	ResetBuffer();
	return out;
}

string TCPCommunicator::remote_address(){
	return socket->remote_endpoint().address().to_string();
}


unsigned short TCPCommunicator::remote_port(){
	return static_cast<unsigned short>(socket->remote_endpoint().port());
}


protocol_type TCPCommunicator::GetProtocol(){
	return tcp;
}





void TCPCommunicator::Connect(boost::asio::io_context & context, string address){
	tcp::resolver resolver(*io_context_ref);
	tcp::resolver::results_type res = resolver.resolve(address, std::to_string(0xBEEF));
	if(res.empty()){
		throw std::runtime_error("Failed to resolve any peers on " + address + ":0xBEEF");
	}
	if(socket->is_open()){
		socket->close();
	}
	boost::system::error_code err;
	boost::asio::connect(*socket, res, err);
	if(err){
		cerr << "[TCPCommunicator::Connect] An error occurred.\n\t" << err.message() << "\n";
		socket->close();
		throw std::runtime_error("[TCPCommunicator::Connect] Failed to connect to " + address + ":0xBEEF\n\t" + err.message());
	}else{
		connected = true;
	}
}


void TCPCommunicator::Connect(boost::asio::io_context &context, std::string address, unsigned int port){
	tcp::resolver resolver(*io_context_ref);
	tcp::resolver::results_type res = resolver.resolve(address, std::to_string(port));
	if(res.empty()){
		throw std::runtime_error("Failed to resolve any peers on " + address + ":" + std::to_string(port));
	}
	
	boost::system::error_code err;
	boost::asio::connect(*socket, res, err);
	if(err){
		cerr << "[TCPCommunicator::Connect] An error occurred.\n\t" << err.message() << "\n";
		socket->close();
		throw std::runtime_error("[TCPCommunicator::Connect] Failed to connect to " + address + ":" + std::to_string(port) + "\n\t" + err.message());
	}else{
		connected = true;
	}
}




