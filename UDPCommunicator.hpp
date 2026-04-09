#pragma once
#include "NetCommunicator.hpp"
#include <boost/asio/ip/udp.hpp>
#include <memory>


class UDPCommunicator : public Communicator{

	private:
		static int nxtID;
		const int id;

		std::unique_ptr<boost::asio::ip::udp::socket> socket;
		boost::asio::ip::udp::endpoint remote_end;
		unsigned short prt;
		std::string addr;
		unsigned int msgSize;
		std::shared_ptr<boost::asio::io_context> io_context_ref;


		int Prime();

		void validateSocket();
		void HandleSend(boost::shared_ptr<std::string> message,
				const boost::system::error_code &err, 
				std::size_t transferred) override;
		
		void StoreMessage(const boost::system::error_code &err,
				std::size_t transferred) override;
		void ResizeBuffer(unsigned int size) override;
		void ResizeOutgoingBuffer(unsigned int size) override;
		void ResetBuffer() override;

		void runContext();
		
	public:
		virtual SocketStateError validateSocketState() const noexcept override;
		virtual SocketStateError categorizeSocketError(const boost::system::error_code& err) const noexcept override;

		std::string GetMessage() override;

		void Connect(boost::asio::io_context & context, std::string address) override;
		void Connect(boost::asio::io_context & context, std::string address, unsigned int port) override;
		UDPCommunicator() = delete;
	UDPCommunicator(std::shared_ptr<boost::asio::io_context> context, unsigned int maxMessage=1024):
		id(nxtID++),
		socket(std::make_unique<boost::asio::ip::udp::socket>(*context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0xBEEF))),
			addr("0.0.0.0"),
			prt(0xBEEF),
			msgSize(maxMessage),
			io_context_ref(context){
				recv_buffer = std::make_unique<std::string>();
				recv_buffer->reserve(msgSize);
				outgoing = boost::make_shared<std::string>(std::string(msgSize, ' '));
				InitializeTimers(*context);
		};

	UDPCommunicator(std::shared_ptr<boost::asio::io_context> context, unsigned short port, unsigned int maxMessage=1024):
		id(nxtID++),
		addr("0.0.0.0"),
			prt(port),
			socket(std::make_unique<boost::asio::ip::udp::socket>(*context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port))),
			msgSize(maxMessage),
			io_context_ref(context){
				recv_buffer = std::make_unique<std::string>();
				recv_buffer->reserve(msgSize);
				outgoing = boost::make_shared<std::string>(std::string(msgSize, ' '));
				InitializeTimers(*context);
		};
		
	UDPCommunicator(std::shared_ptr<boost::asio::io_context> context, std::string address, unsigned short port, unsigned int maxMessage=1024):
		id(nxtID++),
		addr(address),
			prt(port),
			socket(std::make_unique<boost::asio::ip::udp::socket>(*context, boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(address), port))), 
			msgSize(maxMessage),
			io_context_ref(context){
				recv_buffer = std::make_unique<std::string>();
				recv_buffer->reserve(msgSize);
				outgoing = boost::make_shared<std::string>(std::string(msgSize, ' '));
				InitializeTimers(*context);
		};

		UDPCommunicator(UDPCommunicator &other): id(other.id), socket(std::move(other.socket)){
			addr = other.addr;
			prt = other.prt;
			recv_buffer = std::move(other.recv_buffer);
			outgoing = std::move(other.outgoing);
			msgSize = other.msgSize;
			io_context_ref = other.io_context_ref;
		};
	

		UDPCommunicator & operator=(UDPCommunicator & other){
			UDPCommunicator temp(other);
			swap(*this, other);
			return *this;
		};


		UDPCommunicator & operator=(UDPCommunicator &&other){
			UDPCommunicator temp(other);
			swap(*this, other);
			return *this;
		};

		~UDPCommunicator();


		friend void swap(UDPCommunicator first, UDPCommunicator second){
			std::swap(first.addr, second.addr);
			std::swap(first.prt, second.prt);
			std::swap(first.socket, second.socket);
			std::swap(first.remote_end, second.remote_end);
			std::swap(first.recv_buffer, second.recv_buffer);
			std::swap(first.outgoing, second.outgoing);
			std::swap(first.msgSize, second.msgSize);
			std::swap(first.io_context_ref, second.io_context_ref);
		}

		void Send(std::string message, bool async=true) override;
		void Reply(std::string message, bool async=true) override;

		void Receive(bool async=true) override;


		std::string remote_address() override;
		unsigned short remote_port() override;


		protocol_type GetProtocol() override;
		unsigned int maxSize() override;
};

