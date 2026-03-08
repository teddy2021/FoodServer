
#pragma once
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ts/internet.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/make_shared.hpp>
#include <memory>
#include <chrono>
#include <functional>
#include <queue>
#include <optional>
#include <atomic>

#include "Enums.hpp"
#include "CommunicatorExceptions.hpp"

struct AcceptConfig{
	int maxRetries = 3;
	std::chrono::milliseconds initialDelay{100};
	bool exponentialBackoff = true;
	double backoffMultiplier = 2.;
};

struct SendConfig{
	int maxRetries = 3;
	std::chrono::milliseconds initialDelay{100};
	bool exponentialBackoff = true;
	double backoffMultiplier = 2.;
};

enum class AcceptErrorType{ Transient, Resource, Fatal};


class Communicator{

	protected:
		boost::system::error_code error;

		std::unique_ptr<std::string> recv_buffer;
		boost::shared_ptr<std::string> outgoing;
		bool connected = false;
		unsigned int msgSize = 1024;

		OperationConfig opConfig;
		ConnectionStatus connStatus;
		std::queue<std::string> messageQueue;

		std::unique_ptr<boost::asio::deadline_timer> sendTimer;
		std::unique_ptr<boost::asio::deadline_timer> receiveTimer;
		std::unique_ptr<boost::asio::deadline_timer> connectTimer;
		std::optional<std::chrono::steady_clock::time_point> sendStartTime;
		std::optional<std::chrono::steady_clock::time_point> receiveStartTime;
		std::optional<std::chrono::steady_clock::time_point> connectStartTime;
		std::atomic<bool> operationPending{false};

		std::function<void(const CommunicatorException&)> onError;
		std::function<void()> onDisconnected;
		std::function<void()> onReconnected;
		std::function<void(const std::string&)> onMessageQueued;

		virtual void StoreMessage(const boost::system::error_code &err,
				std::size_t transferred){};
		virtual void HandleSend(boost::shared_ptr<std::string> message,
				const boost::system::error_code &err, 
				std::size_t transferred){};

		void updateLastActivity();
		void updateHeartbeat();
		void checkConnectionHealth();
		void processQueue();

		void InitializeTimers(boost::asio::io_context& context);
		void CancelAllTimers();
		void CancelSendTimer();
		void CancelReceiveTimer();
		void CancelConnectTimer();

		template<typename Handler>
		void SetSendTimeout(Handler&& handler){
			if(sendTimer && opConfig.sendTimeout.count() > 0){
				sendTimer->cancel();
				sendTimer->expires_from_now(boost::posix_time::milliseconds(opConfig.sendTimeout.count()));
				sendTimer->async_wait([this, handler = std::move(handler)](const boost::system::error_code& err){
					if(err == boost::asio::error::operation_aborted) return;
					auto elapsed = std::chrono::steady_clock::now() - *sendStartTime;
					auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
					HandleTimeout("Send", ms, std::move(handler));
				});
			}
		}

		template<typename Handler>
		void SetReceiveTimeout(Handler&& handler){
			if(receiveTimer && opConfig.receiveTimeout.count() > 0){
				receiveTimer->cancel();
				receiveTimer->expires_from_now(boost::posix_time::milliseconds(opConfig.receiveTimeout.count()));
				receiveTimer->async_wait([this, handler = std::move(handler)](const boost::system::error_code& err){
					if(err == boost::asio::error::operation_aborted) return;
					auto elapsed = std::chrono::steady_clock::now() - *receiveStartTime;
					auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
					HandleTimeout("Receive", ms, std::move(handler));
				});
			}
		}

		template<typename Handler>
		void SetConnectTimeout(Handler&& handler){
			if(connectTimer && opConfig.connectTimeout.count() > 0){
				connectTimer->cancel();
				connectTimer->expires_from_now(boost::posix_time::milliseconds(opConfig.connectTimeout.count()));
				connectTimer->async_wait([this, handler = std::move(handler)](const boost::system::error_code& err){
					if(err == boost::asio::error::operation_aborted) return;
					auto elapsed = std::chrono::steady_clock::now() - *connectStartTime;
					auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
					HandleTimeout("Connect", ms, std::move(handler));
				});
			}
		}

		template<typename Handler>
		void HandleTimeout(const std::string& operation, std::chrono::milliseconds elapsed, Handler&& handler){
			connected = false;
			connStatus.consecutiveFailures++;
			OperationTimeoutException ex("[Communicator::" + operation + "]", elapsed, 
				operation + " operation timed out after " + std::to_string(elapsed.count()) + "ms");
			if(onError){
				onError(ex);
			}
			handler(ex);
		}

		bool ValidateAddress(const std::string& addr) const;
		bool ValidatePort(unsigned short port) const;
		bool ValidateMessageSize(size_t size) const;

	public:
		virtual SocketStateError validateSocketState() const noexcept;
		virtual SocketStateError categorizeSocketError(const boost::system::error_code& err) const noexcept;
		virtual void ResizeOutgoingBuffer(unsigned int size){};
		virtual void ResizeBuffer(unsigned int size){};
		virtual void ResetBuffer(){};
		virtual std::string GetMessage(){return "";};

		virtual void Send(std::string message){};
		virtual void SendWithRetry(std::string message, int maxRetries = -1);
		
		virtual void Receive(){};
		virtual std::string remote_address(){return "";};
		virtual unsigned short remote_port(){return 0;};

		virtual void Connect(boost::asio::io_context & context, std::string address){};
		virtual void Connect(boost::asio::io_context & context, std::string address, unsigned int port){};

		virtual protocol_type GetProtocol(){return udp;};
		virtual void Reply(std::string message){};

		virtual unsigned int maxSize(){return 0;};

		void setOperationConfig(const OperationConfig& config);
		OperationConfig getOperationConfig() const;

		void setOnError(std::function<void(const CommunicatorException&)> callback);
		void setOnDisconnected(std::function<void()> callback);
		void setOnReconnected(std::function<void()> callback);
		void setOnMessageQueued(std::function<void(const std::string&)> callback);

		ConnectionHealth getConnectionHealth() const;
		ConnectionStatus getConnectionStatus() const;

		bool hasQueuedMessages() const;
		size_t queuedMessageCount() const;
		void clearQueue();
};

