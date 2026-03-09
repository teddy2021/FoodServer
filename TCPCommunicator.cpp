

#include <boost/asio/basic_socket.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/error.hpp>
#include <boost/system/detail/errc.hpp>
#include <boost/system/detail/error_code.hpp>
#include <catch2/catch_tostring.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <iostream>

#include <boost/make_shared.hpp>
#include <boost/bind/bind.hpp>
#include <thread>

#include "TCPCommunicator.hpp"
#include "Logger.hpp"

using std::string;
using boost::shared_ptr;
using boost::asio::buffer;
using boost::asio::ip::tcp;

void TCPCommunicator::HandleSend(boost::shared_ptr<std::string> message, 
		const boost::system::error_code &err,
		std::size_t transferred){
	CancelSendTimer();
	if(err){
		connected = false;
		connStatus.consecutiveFailures++;
		Logger::GetInstance().log("[TCPCommunicator::HandleSend] Failed: " + err.message(), debug_level::ERROR);
		
		auto categorized = categorizeSocketError(err);
		SendException ex(
			(categorized == SocketStateError::RemoteClosed) ? SendError::NetworkError : SendError::NetworkError,
			"[TCPCommunicator::HandleSend]", 
			"Failed to send: " + err.message()
		);
		
		if(onError){
			onError(ex);
		}
		Logger::GetInstance().log("[TCPCommunicator::HandleSend(" + *message + ",__, " + std::to_string(transferred) + ")] Failed to send: " + err.message(), debug_level::ERROR);
	}
	else{
		connected = true;
		connStatus.consecutiveFailures = 0;
		updateLastActivity();
	}
}

void TCPCommunicator::StoreMessage(const boost::system::error_code &err, 
		std::size_t transferred){
	CancelReceiveTimer();
	if(!err){
		ResizeBuffer(transferred);
		connected = true;
		updateHeartbeat();
		updateLastActivity();
		return;
	}
	if(err){
		connected = false;
		connStatus.consecutiveFailures++;
		
		auto categorized = categorizeSocketError(err);
		ReceiveException ex(
			(categorized == SocketStateError::RemoteClosed) ? ReceiveError::ConnectionClosed : ReceiveError::NetworkError,
			"[TCPCommunicator::StoreMessage]", 
			"Error receiving: " + err.message()
		);
		
		if(onError){
			onError(ex);
		}
		throw ex;
	}
}

SocketStateError TCPCommunicator::validateSocketState() const noexcept{
	if(!socket){
		return SocketStateError::NullSocket;
	}
	if(!socket->is_open()){
		return SocketStateError::NotOpen;
	}
	if(!connected){
		return SocketStateError::NotConnected;
	}
	return SocketStateError::None;
}


SocketStateError TCPCommunicator::categorizeSocketError(const boost::system::error_code& err) const noexcept{
	switch(err.value()){
		case boost::asio::error::eof:
			return SocketStateError::RemoteClosed;
		case boost::system::errc::operation_canceled:
			return SocketStateError::RemoteClosed;
		case boost::asio::error::connection_reset:
		case boost::asio::error::connection_aborted:
		case boost::asio::error::broken_pipe:
		case boost::asio::error::timed_out:
		case boost::asio::error::network_reset:
		case boost::asio::error::network_down:
		case boost::asio::error::host_unreachable:
		case boost::asio::error::access_denied:
			return SocketStateError::NetworkError;
		case boost::asio::error::bad_descriptor:
		case boost::asio::error::not_socket:
			return SocketStateError::NotOpen;
		case boost::asio::error::not_connected:
			return SocketStateError::NotConnected;
		default:
			return SocketStateError::NetworkError;
	}
	return SocketStateError::None;
}

TCPCommunicator::~TCPCommunicator(){
	Logger::GetInstance().log("[TCPCommunicator::~TCPCommunicator] destroyed", debug_level::DEBUG);
	CancelAllTimers();
	boost::system::error_code err;
	
	// Clean up socket first
	if(socket && socket->is_open()){  
		socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, err);
		if(err){
			Logger::GetInstance().log("[~TCPCommunicator()]: Failed to shutdown socket: " + err.message(), debug_level::ERROR);
		}
		socket->close(err);
		if(err){
			Logger::GetInstance().log("[~TCPCommunicator()]: Failed to close socket: " + err.message(), debug_level::ERROR);
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
	
	connected = false;
}




void TCPCommunicator::Send(string message){
	Logger::GetInstance().log("[TCPCommunicator::Send] sending message", debug_level::INFO);
	
	auto s_state = validateSocketState();
	if(s_state != SocketStateError::None){
		throw ConnectionException(s_state, "[TCPCommunicator::Send]", "Invalid socket state");
	}
	
	if(!connected){
		throw ConnectionException(SocketStateError::NotConnected, "[TCPCommunicator::Send]", "Not connected to a peer");
	}

	if(!ValidateMessageSize(message.size())){
		throw SendException(SendError::BufferOverflow, "[TCPCommunicator::Send]", 
			"Message size " + std::to_string(message.size()) + " exceeds maximum " + std::to_string(msgSize));
	}
	
	sendStartTime = std::chrono::steady_clock::now();
	auto send_buffer = boost::make_shared<std::string>(message);
	boost::asio::async_write(*socket, buffer(*send_buffer),
		boost::bind(&TCPCommunicator::HandleSend,
			this,
			send_buffer,
			boost::asio::placeholders::error, 
			boost::asio::placeholders::bytes_transferred));
}


void TCPCommunicator::Reply(string message){
	Logger::GetInstance().log("[TCPCommunicator::Reply] replying", debug_level::INFO);
	
	auto s_state = validateSocketState();
	if(s_state != SocketStateError::None){
		throw ConnectionException(s_state, "[TCPCommunicator::Reply]", "Invalid socket state");
	}
	
	if(!connected){
		throw ConnectionException(SocketStateError::NotConnected, "[TCPCommunicator::Reply]", "Not connected to a peer");
	}

	if(!ValidateMessageSize(message.size())){
		throw SendException(SendError::BufferOverflow, "[TCPCommunicator::Reply]", 
			"Message size " + std::to_string(message.size()) + " exceeds maximum " + std::to_string(msgSize));
	}
	
	sendStartTime = std::chrono::steady_clock::now();
	auto send_buffer = boost::make_shared<std::string>(message);
	boost::asio::async_write(*socket, buffer(*send_buffer),
		boost::bind(&TCPCommunicator::HandleSend,
			this,
			send_buffer,
			boost::asio::placeholders::error, 
			boost::asio::placeholders::bytes_transferred));
}





void TCPCommunicator::Accept(){
	Logger::GetInstance().log("[TCPCommunicator::Accept] accepting", debug_level::INFO);
	Accept(acc_con);
}

void TCPCommunicator::Accept(const AcceptConfig& config){
	Logger::GetInstance().log("[TCPCommunicator::Accept] accepting with config", debug_level::INFO);
	boost::system::error_code err;
	validateSocketState();
	int attempts = 0;
	auto delay = config.initialDelay;
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
	while(attempts <= config.maxRetries){
		acceptor->accept(*socket, remote_end, err);
		if(!err){
			connected = true;
			return;
		}

		auto errorType = categorizeAcceptError(err);

		switch(errorType){
			case AcceptErrorType::Fatal:
				throw std::runtime_error("[TCPCommunicator::Accept]: Failed to connect.\n\tFatal error: " + err.message());
				break;
			default:
				attempts += 1;
				if(attempts > config.maxRetries){
					throw std::runtime_error("[TCPCommunicator::Accept]: Failed to connect. \n\tAttempt limit reached. Final error: " + err.message());
					break;
				}
				else{
					std::this_thread::sleep_for(delay);
				}
				if(config.exponentialBackoff){
					delay = std::chrono::milliseconds(
						static_cast<long>(delay.count() * config.backoffMultiplier)
					);
				}
				socket = std::make_unique<tcp::socket>(*io_context_ref);
				break;
			}
	}
}

AcceptErrorType TCPCommunicator::categorizeAcceptError(const boost::system::error_code& err){
	switch(err.value()){
		case boost::system::errc::operation_canceled:
		case boost::system::errc::interrupted:
		case boost::system::errc::connection_aborted:
		case boost::system::errc::operation_would_block:
			return AcceptErrorType::Transient;
		case boost::system::errc::too_many_files_open:
		case boost::system::errc::no_buffer_space:
			return AcceptErrorType::Resource;
		case boost::system::errc::bad_file_descriptor:
		case boost::system::errc::invalid_argument:
		case boost::system::errc::address_in_use:
		default:
			return AcceptErrorType::Fatal;
	}
}



void TCPCommunicator::ResizeBuffer(unsigned int size){
	if(recv_buffer->size() > size){
		recv_buffer->resize(size);
	}
}

void TCPCommunicator::ResetBuffer(){
	recv_buffer->assign(10, '\0');
}





void TCPCommunicator::Receive(){
	Logger::GetInstance().log("[TCPCommunicator::Receive] receiving", debug_level::DEBUG);
	
	auto s_state = validateSocketState();
	if(s_state == SocketStateError::NullSocket || s_state == SocketStateError::NotOpen){
		throw ConnectionException(s_state, "[TCPCommunicator::Receive]", "Socket not available");
	}
	
	if(!connected){
		Accept();
	}
	
	if(!socket->is_open()){
		throw ConnectionException(SocketStateError::NotOpen, "[TCPCommunicator::Receive]", "Socket is not open");
	}
	
	recv_buffer->resize(1024);
	boost::system::error_code err;
	size_t transferred = socket->read_some(
			boost::asio::buffer(*recv_buffer), 
			err);
	
	if(err == boost::asio::error::eof){
		connected = false;
		connStatus.consecutiveFailures++;
		
		ReceiveException ex(ReceiveError::ConnectionClosed, "[TCPCommunicator::Receive]", "Remote closed connection");
		if(onDisconnected){
			onDisconnected();
		}
		if(onError){
			onError(ex);
		}
		throw ex;
	}
	else if (err){
		connected = false;
		connStatus.consecutiveFailures++;
		
		ReceiveException ex(ReceiveError::NetworkError, "[TCPCommunicator::Receive]", "Error: " + err.message());
		if(onError){
			onError(ex);
		}
		throw ex;
	}
	
	recv_buffer->resize(transferred);
	updateLastActivity();
	updateHeartbeat();
}


string TCPCommunicator::GetMessage(){
	string out(*recv_buffer);
	ResetBuffer();
	return out;
}

string TCPCommunicator::remote_address(){
	auto state = validateSocketState();
	if(state != SocketStateError::None){
		throw ConnectionException(state, "[TCPCommunicator::remote_address]", "Socket not in valid state");
	}
	return socket->remote_endpoint().address().to_string();
}


unsigned short TCPCommunicator::remote_port(){
	auto state = validateSocketState();
	if(state != SocketStateError::None){
		throw ConnectionException(state, "[TCPCommunicator::remote_port]", "Socket not in valid state");
	}
	return static_cast<unsigned short>(socket->remote_endpoint().port());
}


protocol_type TCPCommunicator::GetProtocol(){
	return tcp;
}





void TCPCommunicator::Connect(boost::asio::io_context & context, string address){
	Logger::GetInstance().log("[TCPCommunicator::Connect] connecting to: " + address, debug_level::INFO);
	
	auto s_state = validateSocketState();
	if(s_state == SocketStateError::NullSocket){
		throw ConnectionException(SocketStateError::NullSocket, "[TCPCommunicator::Connect]", "Socket is null");
	}
	
	boost::system::error_code err;
	tcp::resolver resolver(*io_context_ref);
	tcp::resolver::results_type res = resolver.resolve(address, std::to_string(0xBEEF), err);
	
	if(err || res.empty()){
		throw ConnectionException("[TCPCommunicator::Connect]", "Failed to resolve: " + address + " - " + err.message());
	}
	
	if(socket->is_open()){
		socket->close(err);
	}
	
	boost::asio::connect(*socket, res, err);
	
	if(err){
		connected = false;
		Logger::GetInstance().log("[TCPCommunicator::Connect] Error: " + err.message(), debug_level::ERROR);
		socket->close(err);
		
		ConnectionException ex(SocketStateError::NetworkError, "[TCPCommunicator::Connect]", "Failed to connect: " + err.message());
		if(onError){
			onError(ex);
		}
		throw ex;
	}
	
	connected = true;
	connStatus.consecutiveFailures = 0;
	updateLastActivity();
	
	if(onReconnected){
		onReconnected();
	}
}


void TCPCommunicator::Connect(boost::asio::io_context &context, std::string address, unsigned int port){
	Logger::GetInstance().log("[TCPCommunicator::Connect] connecting to: " + address + ":" + std::to_string(port), debug_level::INFO);
	
	auto s_state = validateSocketState();
	if(s_state == SocketStateError::NullSocket){
		throw ConnectionException(SocketStateError::NullSocket, "[TCPCommunicator::Connect]", "Socket is null");
	}
	
	boost::system::error_code err;
	tcp::resolver resolver(*io_context_ref);
	tcp::resolver::results_type res = resolver.resolve(address, std::to_string(port), err);
	
	if(err || res.empty()){
		throw ConnectionException("[TCPCommunicator::Connect]", "Failed to resolve: " + address + ":" + std::to_string(port) + " - " + err.message());
	}
	
	if(socket->is_open()){
		socket->close(err);
	}
	
	boost::asio::connect(*socket, res, err);
	
	if(err){
		connected = false;
		Logger::GetInstance().log("[TCPCommunicator::Connect] Error: " + err.message(), debug_level::ERROR);
		socket->close(err);
		
		ConnectionException ex(SocketStateError::NetworkError, "[TCPCommunicator::Connect]", "Failed to connect: " + err.message());
		if(onError){
			onError(ex);
		}
		throw ex;
	}
	
	connected = true;
	connStatus.consecutiveFailures = 0;
	updateLastActivity();
	
	if(onReconnected){
		onReconnected();
	}
}




