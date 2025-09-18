
#pragma once
#include "NetCommunicator.hpp"
#include <boost/date_time/time_defs.hpp>

class TCPCommunicator : public Communicator, std::enable_shared_from_this<TCPCommunicator>{

	private:
		boost::asio::ip::tcp::socket socket;
		boost::asio::ip::tcp::acceptor acceptor;
		boost::asio::ip::tcp::endpoint remote_end;

		void HandleSend(boost::shared_ptr<std::string> message,
				const boost::system::error_code &err,
				std::size_t transferred) override;
		void StoreMessage(const boost::system::error_code &err,
				std::size_t transferred) override;
		void HandleAccept(boost::system::error_code err);
		void HandleConnect(boost::system::error_code err,
				boost::asio::ip::tcp::endpoint ep);
		
	public:
		
		void ResizeBuffer(unsigned int size) override;
		void ResetBuffer() override;
		static std::shared_ptr<TCPCommunicator> create(boost::asio::io_context & io_context){
			return std::shared_ptr<TCPCommunicator>(new TCPCommunicator(io_context));
		}

		void Accept();
		
		void Connect(boost::asio::io_context & context, std::string address) override;
		void Connect(boost::asio::io_context & context, std::string address, unsigned int port) override;
		
		
		TCPCommunicator():
			acceptor(boost::asio::io_context().get_executor()),
			socket(boost::asio::io_context().get_executor()){
				recv_buffer = std::make_unique<std::string>(std::string("0", 10));
				recv_buffer->reserve(10);
				outgoing = boost::make_shared<std::string>(std::string("",1024));
		};

		TCPCommunicator(boost::asio::io_context & con):
			socket(con), 
			remote_end(boost::asio::ip::tcp::v4(),
					0xBEEF),

			acceptor(con, 
					boost::asio::ip::tcp::endpoint(
						boost::asio::ip::tcp::v4(),
					   	0xBEEF))
			{
				recv_buffer = std::make_unique<std::string>(std::string("0", 10));
				outgoing = boost::make_shared<std::string>(std::string(" ", 1024));
			}



		TCPCommunicator(boost::asio::io_context &con, unsigned short port):
			socket(con),
			remote_end(boost::asio::ip::tcp::v4(),
					port),
			acceptor(con,
				   	boost::asio::ip::tcp::endpoint(
						boost::asio::ip::tcp::v4(),
					   	port))
			{
				recv_buffer = std::make_unique<std::string>(std::string("0", 10));
				outgoing = boost::make_shared<std::string>(std::string(" ", 1024));
			}


		TCPCommunicator(boost::asio::io_context &con, std::string address,  unsigned short port):
			socket(con),
			remote_end(boost::asio::ip::tcp::v4(),
					port),
			acceptor(con,
				   	boost::asio::ip::tcp::endpoint(
						boost::asio::ip::make_address(address),
					   	port))
			{
				recv_buffer = std::make_unique<std::string>(std::string("0", 10));
				outgoing = boost::make_shared<std::string>(std::string("", 1024));
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
		}

		void Send(std::string message) override;
		void Reply(std::string message) override;
		
		void Receive() override;
				std::string remote_address() override;
		std::string GetMessage() override;


		unsigned short remote_port() override;
		protocol_type GetProtocol() override;

};

