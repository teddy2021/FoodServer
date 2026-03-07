#include "NetCommunicator.hpp"
#include "Logger.hpp"

SocketStateError Communicator::validateSocketState() const noexcept{
	return SocketStateError::None;
}

SocketStateError Communicator::categorizeSocketError(const boost::system::error_code& err) const noexcept{
	return SocketStateError::NetworkError;
}

void Communicator::updateLastActivity(){
	connStatus.lastActivity = std::chrono::steady_clock::now();
}

void Communicator::updateHeartbeat(){
	connStatus.lastHeartbeat = std::chrono::steady_clock::now();
}

void Communicator::checkConnectionHealth(){
	auto now = std::chrono::steady_clock::now();
	
	if(!connected){
		connStatus.health = ConnectionHealth::Unhealthy;
		return;
	}

	if(connStatus.consecutiveFailures >= connStatus.maxFailuresBeforeDisconnect){
		connStatus.health = ConnectionHealth::Unhealthy;
		if(onDisconnected){
			onDisconnected();
		}
		return;
	}

	if(opConfig.enableHeartbeat){
		auto timeSinceHeartbeat = std::chrono::duration_cast<std::chrono::milliseconds>(
			now - connStatus.lastHeartbeat);
		
		if(timeSinceHeartbeat > opConfig.heartbeatInterval){
			connStatus.health = ConnectionHealth::Degraded;
		}
		else{
			connStatus.health = ConnectionHealth::Healthy;
		}
	}
	else{
		connStatus.health = ConnectionHealth::Healthy;
	}
}

void Communicator::processQueue(){
	while(!messageQueue.empty() && connected){
		auto msg = messageQueue.front();
		messageQueue.pop();
		try{
			Send(msg);
		}
		catch(const CommunicatorException& e){
			messageQueue.push(msg);
			connStatus.consecutiveFailures++;
			throw;
		}
	}
}

void Communicator::SendWithRetry(std::string message, int maxRetries){
	auto state = validateSocketState();
	
	if(state != SocketStateError::None){
		if(opConfig.enableAutoReconnect && (state == SocketStateError::NotConnected || 
										  state == SocketStateError::NotOpen)){
			ConnectionException ex(state, "[SendWithRetry]", "Attempting to reconnect");
			if(onError){
				onError(ex);
			}
		}
		
		SendException ex(SendError::NotConnected, "[SendWithRetry]", 
			"Cannot send: " + std::to_string(static_cast<int>(state)));
		if(onError){
			onError(ex);
		}
		throw ex;
	}

	int attempts = 0;
	int limit = (maxRetries < 0) ? opConfig.maxRetries : maxRetries;
	
	while(attempts <= limit){
		try{
			Send(message);
			connStatus.consecutiveFailures = 0;
			updateLastActivity();
			return;
		}
		catch(const CommunicatorException& e){
			attempts++;
			connStatus.consecutiveFailures++;
			
			if(onError){
				onError(e);
			}
			
			if(attempts > limit){
				SendException ex(SendError::NetworkError, "[SendWithRetry]",
					"Max retries exceeded: " + std::string(e.what()));
				throw ex;
			}
		}
	}
}

void Communicator::setOperationConfig(const OperationConfig& config){
	opConfig = config;
}

OperationConfig Communicator::getOperationConfig() const{
	return opConfig;
}

void Communicator::setOnError(std::function<void(const CommunicatorException&)> callback){
	onError = callback;
}

void Communicator::setOnDisconnected(std::function<void()> callback){
	onDisconnected = callback;
}

void Communicator::setOnReconnected(std::function<void()> callback){
	onReconnected = callback;
}

void Communicator::setOnMessageQueued(std::function<void(const std::string&)> callback){
	onMessageQueued = callback;
}

ConnectionHealth Communicator::getConnectionHealth() const{
	return connStatus.health;
}

ConnectionStatus Communicator::getConnectionStatus() const{
	return connStatus;
}

bool Communicator::hasQueuedMessages() const{
	return !messageQueue.empty();
}

size_t Communicator::queuedMessageCount() const{
	return messageQueue.size();
}

void Communicator::clearQueue(){
	while(!messageQueue.empty()){
		messageQueue.pop();
	}
}

void Communicator::InitializeTimers(boost::asio::io_context& context){
	sendTimer = std::make_unique<boost::asio::deadline_timer>(context);
	receiveTimer = std::make_unique<boost::asio::deadline_timer>(context);
	connectTimer = std::make_unique<boost::asio::deadline_timer>(context);
}

void Communicator::CancelAllTimers(){
	boost::system::error_code err;
	if(sendTimer && sendTimer->cancel(err), err){
		Logger::GetInstance().log("[Communicator::CancelAllTimers] Failed to cancel send timer: " + err.message(), debug_level::WARN);
	}
	if(receiveTimer && receiveTimer->cancel(err), err){
		Logger::GetInstance().log("[Communicator::CancelAllTimers] Failed to cancel receive timer: " + err.message(), debug_level::WARN);
	}
	if(connectTimer && connectTimer->cancel(err), err){
		Logger::GetInstance().log("[Communicator::CancelAllTimers] Failed to cancel connect timer: " + err.message(), debug_level::WARN);
	}
}

void Communicator::CancelSendTimer(){
	boost::system::error_code err;
	if(sendTimer && sendTimer->cancel(err), err){
		Logger::GetInstance().log("[Communicator::CancelSendTimer] Failed to cancel: " + err.message(), debug_level::WARN);
	}
}

void Communicator::CancelReceiveTimer(){
	boost::system::error_code err;
	if(receiveTimer && receiveTimer->cancel(err), err){
		Logger::GetInstance().log("[Communicator::CancelReceiveTimer] Failed to cancel: " + err.message(), debug_level::WARN);
	}
}

void Communicator::CancelConnectTimer(){
	boost::system::error_code err;
	if(connectTimer && connectTimer->cancel(err), err){
		Logger::GetInstance().log("[Communicator::CancelConnectTimer] Failed to cancel: " + err.message(), debug_level::WARN);
	}
}

bool Communicator::ValidateAddress(const std::string& addr) const{
	if(addr.empty()){
		return false;
	}
	boost::system::error_code err;
	boost::asio::ip::make_address(addr, err);
	return !err;
}

bool Communicator::ValidatePort(unsigned short port) const{
	return port != 0;
}

bool Communicator::ValidateMessageSize(size_t size) const{
	return size > 0 && size <= msgSize;
}
