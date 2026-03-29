
#pragma once
#include "NetCommunicator.hpp"
#include <boost/date_time/time_defs.hpp>
#include <boost/system/detail/error_code.hpp>
#include <cstddef>
#include <memory>



class TCPCommunicator : public Communicator, std::enable_shared_from_this<TCPCommunicator>{

	private:
		const AcceptConfig acc_con;
		std::unique_ptr<boost::asio::ip::tcp::socket> socket;
		std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;
		boost::asio::ip::tcp::endpoint remote_end;
		std::shared_ptr<boost::asio::io_context> io_context_ref;
		bool is_server_mode;
		
		void initializeSocket(boost::asio::io_context& con);
		void initializeAcceptor(boost::asio::io_context& con, unsigned short port);
		SocketStateError validateSocketState() const noexcept override;
		SocketStateError categorizeSocketError(const boost::system::error_code& err) const noexcept override;
		
		void HandleSend(boost::shared_ptr<std::string> message,
				const boost::system::error_code &err, 
				std::size_t transferred) override;
		
		void StoreMessage(const boost::system::error_code &err,
				std::size_t transferred) override;

		void HandleAccept(const boost::system::error_code &err,
				std::size_t transferred);
		
	public:
		
		void ResizeBuffer(unsigned int size) override;
		void ResetBuffer() override;
		static std::shared_ptr<TCPCommunicator> create(std::shared_ptr<boost::asio::io_context> io_context){
			return std::shared_ptr<TCPCommunicator>(new TCPCommunicator(io_context));
		}

		bool isServerMode() const { return is_server_mode; }
		bool isConnected() const { return connected && socket && socket->is_open(); }

		void Accept(bool async=false) override;
		void Accept(const AcceptConfig& config, bool async=false);
		AcceptErrorType categorizeAcceptError(const boost::system::error_code& err);

		
		void Connect(boost::asio::io_context & context, std::string address) override;
		void Connect(boost::asio::io_context & context, std::string address, unsigned int port) override;
		
		
		TCPCommunicator() = delete;

		TCPCommunicator(std::shared_ptr<boost::asio::io_context> con):
			socket(std::make_unique<boost::asio::ip::tcp::socket>(*con)), 
			remote_end(boost::asio::ip::tcp::v4(), 0xBEEF),
			acceptor(nullptr),
			io_context_ref(con),
			is_server_mode(false)
			{
				recv_buffer = std::make_unique<std::string>(std::string(msgSize, ' '));
				outgoing = boost::make_shared<std::string>(std::string(msgSize, ' '));
				InitializeTimers(*con);
			}



		TCPCommunicator(std::shared_ptr<boost::asio::io_context> con, unsigned short port):
			socket(std::make_unique<boost::asio::ip::tcp::socket>(*con)),
			remote_end(boost::asio::ip::tcp::v4(), port),
			acceptor(std::make_unique<boost::asio::ip::tcp::acceptor>(*con, 
				boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))),
			io_context_ref(con),
			is_server_mode(true)
			{
				recv_buffer = std::make_unique<std::string>(std::string(msgSize, ' '));
				outgoing = boost::make_shared<std::string>(std::string(msgSize, ' '));
				InitializeTimers(*con);
			}

	TCPCommunicator(std::shared_ptr<boost::asio::io_context> con, std::string address,  unsigned short port):
			socket(std::make_unique<boost::asio::ip::tcp::socket>(*con)),
			remote_end(boost::asio::ip::tcp::v4(), port),
			acceptor(std::make_unique<boost::asio::ip::tcp::acceptor>(*con,
				boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port))),
			io_context_ref(con),
			is_server_mode(true)
			{
				recv_buffer = std::make_unique<std::string>(std::string(msgSize, ' '));
				outgoing = boost::make_shared<std::string>(std::string(msgSize, ' '));
				InitializeTimers(*con);
			}

		TCPCommunicator(TCPCommunicator &other):socket(std::move( other.socket )), acceptor(std::move(other.acceptor)), remote_end(other.remote_end){};


		TCPCommunicator & operator=(TCPCommunicator & other){
			TCPCommunicator temp(other);
			swap(*this, temp);
			return *this;
		};


		TCPCommunicator & operator=(TCPCommunicator &&other){
			TCPCommunicator temp(other);
			swap(*this, other);
			return *this;
		};

		~TCPCommunicator();

		friend void swap(TCPCommunicator first, TCPCommunicator second){
			std::swap(first.socket, second.socket);
			std::swap(first.acceptor, second.acceptor);
			std::swap(first.remote_end, second.remote_end);
			std::swap(first.io_context_ref, second.io_context_ref);
		}

		void Send(std::string message, bool async=true) override;
		void Reply(std::string message) override;
		
		void Receive(bool async=true) override;
				std::string remote_address() override;
		std::string GetMessage() override;


		unsigned short remote_port() override;
		protocol_type GetProtocol() override;

		unsigned int maxSize() override{ return 0; };
};

