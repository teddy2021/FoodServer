
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/ts/internet.hpp>
#include <boost/bind/bind.hpp>
#include <boost/lexical_cast.hpp>

#include <cmath>
#include <cstddef>
#include <ratio>
#include <regex>
#include <stdexcept>
#include <string.h>
#include <string>
#include <thread>

#include "UDPCommunicator.hpp"
#include "CommunicatorExceptions.hpp"
#include "Enums.hpp"
#include "Logger.hpp"
using boost::asio::buffer;
using std::string;
using boost::shared_ptr;


void UDPCommunicator::runContext(){
	if(io_context_ref->stopped()){
		io_context_ref->restart();
	}
	while(io_context_ref->poll_one()){}
}

SocketStateError UDPCommunicator::validateSocketState() const noexcept{
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

SocketStateError UDPCommunicator::categorizeSocketError(const boost::system::error_code& err) const noexcept{
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
		case boost::asio::error::message_size:
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

UDPCommunicator::~UDPCommunicator(){
	Logger::GetInstance().log("[UDPCommunicator::~UDPCommunicator] destroyed", debug_level::DEBUG);
	//if(socket->is_open()) {
	//	auto err = socket->cancel(error);
	//	if(error && !err){
	//		cerr << "[~UDPCommunicator()]: Failed to cancel async operations in ~UDPCommunicator;\n\t" << 
	//			error.message() << "\n";
	//	}
	//	if(err && !error){
	//		cerr << "[~UDPCommunicator()]: Failed to cancel async operations\n\t" << err.message() << "\n";
	//	}
	//	socket->close();
	//	socket->release();
	//}
	if(outgoing != nullptr && outgoing->size() > 0) {
	   	outgoing->erase(); 
	}
	if(recv_buffer != nullptr && recv_buffer->size() > 0) {
	   	recv_buffer->erase(); 
	}
}

string UDPCommunicator::GetMessage(){
	if(!recv_buffer){
		Logger::GetInstance().log("[UDPCommunicator::GetMessage]: null message storage pointer", debug_level::ERROR);
		throw std::runtime_error("[UDPCommunicator::GetMessage]: null message storage pointer");
	}
	string out(*recv_buffer);
	out.shrink_to_fit();
	if(out.empty()){
		return "";
	}
	return out;
}

void UDPCommunicator::ResizeBuffer(unsigned int size){
	Logger::GetInstance().log("[UDPCommunicator::ResizeBuffer] resizing from " + std::to_string(recv_buffer->size()) + " to " + std::to_string(size), debug_level::INFO);
	if(size > msgSize){
		Logger::GetInstance().log("[UDPCommunicator::ResizeBuffer(" + std::to_string(size) + ")]: Size is greater than max allowable size (" + std::to_string(msgSize) + ").", debug_level::ERROR);
		throw std::runtime_error("[UDPCommunicator::ResizeBuffer(" + std::to_string(size) + ")]: Size is greater than max allowable size (" + std::to_string(msgSize) + ").");
	}
	if(recv_buffer->size() < size){
		recv_buffer->resize(size);
	}
	else{
		recv_buffer->shrink_to_fit();
		*recv_buffer = recv_buffer->substr(0, size);
	}
}

void UDPCommunicator::ResetBuffer(){
	Logger::GetInstance().log("[UDPCommunicator::ResetBuffer] clearing and setting incoming buffer size to " + std::to_string(msgSize), debug_level::INFO);
	recv_buffer->clear();
	ResizeBuffer(msgSize);
}

void UDPCommunicator::ResizeOutgoingBuffer(unsigned int size){
	outgoing->resize(size);
}


void UDPCommunicator::HandleSend(boost::shared_ptr<std::string> message, 
		const boost::system::error_code &err,
		std::size_t transferred){
	CancelSendTimer();
	if(err){
		connected = false;
		connStatus.consecutiveFailures++;
		Logger::GetInstance().log("[UDPCommunicator::HandleSend] Failed: " + err.message(), debug_level::ERROR);
		
		auto categorized = categorizeSocketError(err);
		SendException ex(
			SendError::NetworkError,
			"[UDPCommunicator::HandleSend]", 
			"Failed to send: " + err.message()
		);
		
		if(onError){
			onError(ex);
		}
		Logger::GetInstance().log("[UDPCommunicator::HandleSend(" + *message + ",__, " + std::to_string(transferred) + ")] Failed: " + err.message(), debug_level::ERROR);
	}
	else{
		connected = true;
		connStatus.consecutiveFailures = 0;
		updateLastActivity();
	}
}

void UDPCommunicator::Send(string message){
	Logger::GetInstance().log("[UDPCommunicator::Send] sending message", debug_level::INFO);
	auto s_state = validateSocketState();
	
	switch(s_state){
		case SocketStateError::NullSocket:
			Logger::GetInstance().log("[UDPCommunicator::Send]: NullSocket error.", debug_level::ERROR);
			throw ConnectionException(SocketStateError::NullSocket, "[UDPCommunicator::Send]", "Socket is null");
		case SocketStateError::NotOpen:
			Logger::GetInstance().log("[UDPCommunicator::Send]: NotOpen error.", debug_level::ERROR);
			throw ConnectionException(SocketStateError::NotOpen, "[UDPCommunicator::Send]", "Socket is not open");
		case SocketStateError::NotConnected:
			Logger::GetInstance().log("[UDPCommunicator::Send]: NotConnected error.", debug_level::ERROR);
			throw ConnectionException(SocketStateError::NotConnected, "[UDPCommunicator::Send]", "Not connected to endpoint");
		default:
			break;
	}
	
	if(!connected){
		Logger::GetInstance().log("[UDPCommunicator::Send] Unable to send without an endpoint.", debug_level::ERROR);
		throw ConnectionException(SocketStateError::NotConnected, "[UDPCommunicator::Send]", "Unable to send without an endpoint");
	}

	if(!ValidateMessageSize(message.size())){
		throw SendException(SendError::BufferOverflow, "[UDPCommunicator::Send]", 
			"Message size " + std::to_string(message.size()) + " exceeds maximum " + std::to_string(msgSize));
	}
	
	ResizeOutgoingBuffer(message.size() + 1);
	*outgoing = message;
	outgoing->shrink_to_fit();
	sendStartTime = std::chrono::steady_clock::now();
	SetSendTimeout([](const OperationTimeoutException& e){
			Logger::GetInstance().log("[UDPCommunicator::Send] operation timed out.", debug_level::ERROR);
			});
	socket->async_send_to(buffer(*outgoing),
			remote_end,
			boost::bind(&UDPCommunicator::HandleSend,
				this,
				outgoing,
				boost::asio::placeholders::error, 
				boost::asio::placeholders::bytes_transferred));
	runContext();

}

void UDPCommunicator::Reply(string message){
	Logger::GetInstance().log("[UDPCommunicator::Reply] replying", debug_level::INFO);
	
	auto s_state = validateSocketState();
	if(s_state != SocketStateError::None){
		throw ConnectionException(s_state, "[UDPCommunicator::Reply]", "Invalid socket state");
	}
	
	if(!connected){
		throw ConnectionException(SocketStateError::NotConnected, "[UDPCommunicator::Reply]", "Cannot reply without first being contacted");
	}

	if(!ValidateMessageSize(message.size())){
		throw SendException(SendError::BufferOverflow, "[UDPCommunicator::Reply]", 
			"Message size " + std::to_string(message.size()) + " exceeds maximum " + std::to_string(msgSize));
	}
	
	outgoing->resize(message.size());
	outgoing->shrink_to_fit();
	*outgoing = message;
	sendStartTime = std::chrono::steady_clock::now();
	SetSendTimeout([](const OperationTimeoutException& e){
			Logger::GetInstance().log("[UDPCommunicator::Reply] operation timed out", debug_level::ERROR);
			});
	socket->async_send_to(buffer(*outgoing),
			remote_end,	
			boost::bind(&UDPCommunicator::HandleSend,
				this,
				outgoing,
				boost::asio::placeholders::error, 
				boost::asio::placeholders::bytes_transferred));
	runContext();
}


void UDPCommunicator::Receive(bool async){
	Logger::GetInstance().log("[UDPCommunicator::Receive] receiving", debug_level::DEBUG);
	
	auto s_state = validateSocketState();
	if(s_state != SocketStateError::None && s_state != SocketStateError::NotConnected){
		throw ConnectionException(s_state, "[UDPCommunicator::Receive]", "Invalid socket state");
	}
	
	ResetBuffer();
	receiveStartTime = std::chrono::steady_clock::now();
	SetReceiveTimeout([](const OperationTimeoutException e){
		Logger::GetInstance().log("[UDPCommunicator::Receive] operation timed out", debug_level::ERROR);
		});
	if(!connected){
		Logger::GetInstance().log("[UDPCommunicator::Receive] Receiving on unconnected socket.", debug_level::DEBUG);
	socket->async_receive_from(buffer(*recv_buffer),
			remote_end,
			boost::bind(&UDPCommunicator::StoreMessage, 
				this, 
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
	else{
		size_t transferred;
		Logger::GetInstance().log("[UDPCommunicator::Receive] Receiving on connected socket.", debug_level::DEBUG);
		if(async){
			socket->async_receive(buffer(*recv_buffer),
				boost::bind(&UDPCommunicator::StoreMessage,
					this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else{
			while((transferred = socket->receive(buffer(*recv_buffer))) == 0){
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
	}
	updateLastActivity();
	runContext();
}

void UDPCommunicator::StoreMessage(const boost::system::error_code &err, 
		std::size_t transferred){
	Logger::GetInstance().log("[UDPCommunicator::StoreMessage] storing message.", debug_level::INFO);
	CancelReceiveTimer();
	std::regex pattern("^u[:digit:]+$", std::regex_constants::extended);
	if(!err && std::regex_match(*recv_buffer, pattern)){
		connected = true;
		Logger::GetInstance().log("[UDPCommunicator::StoreMessage] Split message requested.", debug_level::INFO);
		int upper = std::stoi(recv_buffer->substr(1, recv_buffer->size()));
		string substr = outgoing->substr(upper, outgoing->size() - upper);
		Send(substr);
		return;
	}
	CancelReceiveTimer();
	if(!err && transferred > msgSize){
		connected = true;
		Logger::GetInstance().log("[UDPCommunicator::StoreMessage] Requesting split message.", debug_level::INFO);
		auto remainder = transferred - msgSize;
		Reply("u" + std::to_string(remainder));
		return;
	}
	else if(!err && transferred <= msgSize){
		Logger::GetInstance().log("[UDPCommunicator::StoreMessage] Successfully received " + *recv_buffer, debug_level::INFO);
		ResizeBuffer(transferred);
		connected = true;
		updateHeartbeat();
		updateLastActivity();
	}
	else if(err){
		auto categorized = categorizeSocketError(err);
		connStatus.consecutiveFailures++;
		connected = false;
		ResetBuffer();
		throw ReceiveException(
			(categorized == SocketStateError::RemoteClosed) ? ReceiveError::ConnectionClosed : ReceiveError::NetworkError,
			"[UDPCommunicator::StoreMessage]", 
			"Error receiving: " + err.message()
		);
	}
}



void UDPCommunicator::Connect(boost::asio::io_context & context, string address){
	Logger::GetInstance().log("[UDPCommunicator::Connect] connecting to: " + address, debug_level::INFO);
	
	auto s_state = validateSocketState();
	if(s_state == SocketStateError::NullSocket){
		Logger::GetInstance().log("[UDPCommunicator::Connect] Socket is null", debug_level::ERROR);
		throw ConnectionException(SocketStateError::NullSocket, "[UDPCommunicator::Connect]", "Socket is null");
	}
	
	boost::system::error_code err;
	boost::asio::ip::udp::resolver r(*io_context_ref);
	boost::asio::ip::udp::resolver::results_type res = r.resolve(address, std::to_string(0xBEEF), err);
	
	if(err || res.empty()){
		Logger::GetInstance().log("[UDPCommunicator::Connect] Failed to resolve address: " + address + " - " + err.message(), debug_level::ERROR);
		throw ConnectionException("[UDPCommunicator::Connect]", "Failed to resolve address: " + address + " - " + err.message());
	}
	
	for(auto it = res.begin() ; it != res.end(); it ++){
		if(it->endpoint().address().is_v4()){
			remote_end = it->endpoint();
			break;
		}
	}
	
	connected = true;
	connStatus.consecutiveFailures = 0;
	updateLastActivity();
}

void UDPCommunicator::Connect(boost::asio::io_context & context,string address, unsigned int port){
	Logger::GetInstance().log("[UDPCommunicator::Connect] connecting to: " + address + ":" + std::to_string(port), debug_level::INFO);
	
	auto s_state = validateSocketState();
	if(s_state == SocketStateError::NullSocket){
		throw ConnectionException(SocketStateError::NullSocket, "[UDPCommunicator::Connect]", "Socket is null");
	}
	
	boost::system::error_code err;
	boost::asio::ip::udp::resolver r(*io_context_ref);
	boost::asio::ip::udp::resolver::results_type res = r.resolve(address, std::to_string(port), err);
	
	if(err || res.empty()){
		throw ConnectionException("[UDPCommunicator::Connect]", "Failed to resolve: " + address + ":" + std::to_string(port) + " - " + err.message());
	}
	
	for(auto it = res.begin() ; it != res.end(); it ++){
		if(it->endpoint().address().is_v4() && it->endpoint().port() == port){
			remote_end = it->endpoint();
			break;
		}
	}
	connected = true;
	connStatus.consecutiveFailures = 0;
	updateLastActivity();
}


string UDPCommunicator::remote_address(){
	validateSocketState();
	return remote_end.address().to_string();
}

unsigned short UDPCommunicator::remote_port(){
	validateSocketState();
	return static_cast<unsigned short>(remote_end.port());
}

protocol_type UDPCommunicator::GetProtocol(){
	return udp;
}

unsigned int UDPCommunicator::maxSize(){
	return msgSize;
}
