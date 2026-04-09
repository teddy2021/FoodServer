#include "../NetMessenger.hpp"
#include "../UDPCommunicator.hpp"
#include "../TCPCommunicator.hpp"
#include "../dbConnector.hpp"
#include "../FileManager.hpp"
#include "../MessageManager.hpp"
#include "../Server.hpp"
#include "../Enums.hpp"
#include "../Logger.hpp"

#include <boost/asio/io_context.hpp>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <random>
#include <iostream>
#include <bits/stdc++.h>
#include <thread>
#include <bits/stdc++.h>
#include <boost/algorithm/string/trim.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

using boost::algorithm::trim;
using std::string;
using std::u32string;
using std::vector;
using Catch::Matchers::Equals;

#define REQUIRE_FILE_EXISTS(path) \
    do { \
        auto p = std::filesystem::absolute(path); \
        INFO("Checking file: " << p); \
        REQUIRE(std::filesystem::exists(p)); \
        REQUIRE((std::filesystem::status(p).permissions() & std::filesystem::perms::owner_read) != std::filesystem::perms::none); \
    } while(0)

struct SocketGuard {
    ~SocketGuard() {
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }
    int fd = -1;
};

bool IsPortAvailable(int port) {
    SocketGuard sock;
    sock.fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock.fd < 0) return false;
    
    int opt = 1;
    setsockopt(sock.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    int result = bind(sock.fd, (sockaddr*)&addr, sizeof(addr));
    return result == 0;
}

bool ValidateDatabase() {
    try {
        DBConnector con("jdbc:mariadb://localhost:3306/FoodServer");
        return true;
    } catch (const std::exception& e) {
        INFO("Database validation failed: " << e.what());
        return false;
    } catch (...) {
        INFO("Database validation failed: unknown error");
        return false;
    }
}

bool ValidateFileFixtures() {
    std::vector<std::string> requiredFiles = {
        "pages/index.html"
    };
    for (const auto& file : requiredFiles) {
        auto p = std::filesystem::absolute(file);
        INFO("Checking file: " << p);
        if (!std::filesystem::exists(p)) {
            INFO("File does not exist: " << p);
            return false;
        }
    }
    return true;
}

bool ValidateNetworkAvailability() {
    if (!IsPortAvailable(38080)) return false;
    if (!IsPortAvailable(38081)) return false;
    return true;
}

bool ValidateWorkingDirectory() {
    auto cwd = std::filesystem::current_path();
    INFO("Working directory: " << cwd);
    return std::filesystem::exists("test_src") || std::filesystem::exists("pages");
}


TEST_CASE("Validate test environment", "[setup]"){
    Logger::GetInstance().SetLevel(debug_level::ERROR);
    SECTION("Database validation"){
        REQUIRE(ValidateDatabase());
    }
    
    SECTION("File fixtures validation"){
        REQUIRE(ValidateFileFixtures());
    }
    
    SECTION("Network availability"){
        REQUIRE(ValidateNetworkAvailability());
    }
    
    SECTION("Working directory"){
        REQUIRE(ValidateWorkingDirectory());
    }
}


struct NetMessengerFixture {
	std::vector<std::unique_ptr<NetMessenger>> messengers;
	std::vector<Recipient> recipients;
	unsigned short base_port = 38080;
	
	~NetMessengerFixture() = default;
	
	NetMessenger& createMessenger(protocol_type type, unsigned short port_offset = 0) {
		messengers.emplace_back(std::make_unique<NetMessenger>(type, base_port + port_offset));
		return *messengers.back();
	}
	
	Recipient createRecipient(unsigned short port_offset = 0) {
		Recipient rec = std::make_shared<recipient>("localhost", base_port + port_offset);
		recipients.push_back(rec);
		return rec;
	}
};

TEST_CASE("Testing Constructors"){
	Logger::GetInstance().SetLevel(debug_level::ERROR);
	auto con = std::make_shared<boost::asio::io_context>();
	SECTION("Testing UDPCommunicator","[UDP][Constructor]"){
		con->run();

		REQUIRE_NOTHROW(std::make_unique<UDPCommunicator>(con, static_cast<unsigned short>(8081)));
		REQUIRE_NOTHROW(std::make_unique<UDPCommunicator>(con, "127.0.0.1", 8083));

		std::cout << "Moving on from nothrows to throws.\n";
		auto com3 = std::make_unique<UDPCommunicator>(con);
		REQUIRE_THROWS((std::make_unique<UDPCommunicator>(con)));
		REQUIRE_THROWS((std::make_unique<UDPCommunicator>(con, static_cast<unsigned short>(0xBEEF))));
		REQUIRE_THROWS((std::make_unique<UDPCommunicator>(con, "0.0.0.0", 0xBEEF)));
		REQUIRE_THROWS((std::make_unique<UDPCommunicator>(con, "localhost", 8080)));
	}

	SECTION("Testing TCPCommunicator","[TCP][Constructor]"){
		TCPCommunicator com1(con);
		REQUIRE_NOTHROW(com1 = TCPCommunicator(con));

		REQUIRE_NOTHROW(com1 = TCPCommunicator(con, 8080));
		REQUIRE_NOTHROW(std::make_unique<TCPCommunicator>(con, 8081));

		REQUIRE_NOTHROW(com1 = TCPCommunicator(con, "127.0.0.1", 8082));
		REQUIRE_NOTHROW(std::make_unique<TCPCommunicator>(con, "127.0.0.1", 8083));
	}

	SECTION("Testing DBConnector", "[DB][Construtor]"){

		DBConnector db;
		REQUIRE_NOTHROW(db = DBConnector("jdbc:mariadb://localhost:3306/FoodServerTest") );
		REQUIRE_THROWS(db = DBConnector("hello"));
	}

	SECTION("Testing MessageManager", "[Storage][Constructor]"){
		MessageManager msg;
		REQUIRE_NOTHROW(msg = MessageManager());
	}

	SECTION("Testing FileManager", "[FileManager][Constructor]"){
		REQUIRE_NOTHROW(std::make_unique<FileManager>("test_src/test.cpp"));
		REQUIRE_THROWS((std::make_unique<FileManager>("astd")));
		REQUIRE_NOTHROW(std::make_unique<FileManager>("FileManager.hpp"));
	}

	SECTION("Testing NetMessenger", "[NetMessenger][Construtor]"){
		std::unique_ptr<NetMessenger> nm_ptr;
		NetMessenger nm;
		REQUIRE_NOTHROW(nm = NetMessenger(udp));
		REQUIRE_NOTHROW(nm = NetMessenger(tcp));
		REQUIRE_NOTHROW(nm = NetMessenger(udp, 8080));
		REQUIRE_NOTHROW(nm = NetMessenger(tcp, 8081));
		REQUIRE_NOTHROW(nm = NetMessenger( udp, "0.0.0.0", 8082) );
		REQUIRE_NOTHROW(nm = NetMessenger(tcp, "0.0.0.0", 8083));
		REQUIRE_NOTHROW( nm = NetMessenger() );
		REQUIRE_NOTHROW(nm_ptr = std::make_unique<NetMessenger>(udp));
		CAPTURE(nm);
		REQUIRE_THROWS(nm = NetMessenger(udp));
		nm_ptr = nullptr;

		REQUIRE_NOTHROW(nm_ptr = std::make_unique<NetMessenger>(tcp));
		CAPTURE(nm);
		REQUIRE_THROWS(nm = NetMessenger(tcp));
		nm_ptr = nullptr;

		REQUIRE_NOTHROW(nm_ptr = std::make_unique<NetMessenger>(udp));
		CAPTURE(nm);
		REQUIRE_THROWS(nm = NetMessenger(udp, 0xDEAD));
		CAPTURE(nm);
		REQUIRE_THROWS(nm = NetMessenger(udp, "0.0.0.0", 0xDEAD));

		CAPTURE(nm);
		REQUIRE_THROWS(nm = NetMessenger(udp, "hello", 2021));
		CAPTURE(nm);
		REQUIRE_THROWS(nm = NetMessenger(tcp, "hello", 2021));
	}

	SECTION("Testing Server", "[Server],[Construtor]"){
		Server s;
		DBConnector db;
		auto udp_c = std::make_shared<NetMessenger>(udp, 8080);
		auto tcp_c = std::make_shared<NetMessenger>(tcp, 8080);
		REQUIRE_NOTHROW(s = Server());
		REQUIRE_NOTHROW(s = Server());
		REQUIRE_NOTHROW(s = Server(*udp_c));
		REQUIRE_NOTHROW(s = Server(*tcp_c));

		REQUIRE_NOTHROW(std::make_unique<Server>(*udp_c));
		REQUIRE_NOTHROW(s = Server(*udp_c));

		REQUIRE_NOTHROW(std::make_unique<Server>(*tcp_c));
		REQUIRE_NOTHROW(s = Server(*tcp_c));
	}

}

TEST_CASE("Testing connection establishment"){
	Logger::GetInstance().SetLevel(debug_level::ERROR);
	SECTION("Testing UDPCommunicator", "[UDP][connect]"){
		auto con = std::make_shared<boost::asio::io_context>();
		con->run();
		UDPCommunicator comm1(con, static_cast<unsigned short>(8080));
		UDPCommunicator comm2(con, static_cast<unsigned short>(8081));

		REQUIRE_NOTHROW(comm1.Connect(*con, "localhost", 8081));
		CAPTURE(comm1.remote_address());
		REQUIRE(comm1.remote_address() == "127.0.0.1");
		CAPTURE(comm1.remote_port());
		REQUIRE(comm1.remote_port() == 8081);

		REQUIRE_THROWS(comm1.Connect(*con, "hostlocal"));
		REQUIRE_THROWS(comm1.Connect(*con, "hostlocal", 1234));
		REQUIRE_THROWS(comm1.Connect(*con, "localhost", -1));
	}


	SECTION("Testing TCPCommunicator", "[TCP][connect]"){
		auto con1 = std::make_shared<boost::asio::io_context>();
		auto con2 = std::make_shared<boost::asio::io_context>();
		auto comm1 = std::make_unique<TCPCommunicator>(con1);
		auto comm2 = std::make_unique<TCPCommunicator>(con2, 8081);

		REQUIRE_NOTHROW(comm1->Connect(*con1, "localhost", 8081));
		REQUIRE_NOTHROW(comm2->Accept());
		CAPTURE(comm1->remote_address());
		REQUIRE(comm1->remote_address() == "127.0.0.1");
		CAPTURE(comm1->remote_port());
		REQUIRE(comm1->remote_port() == 8081);
	}
}

TEST_CASE("Testing UDPCommunicator", "[UDP]"){
	Logger::GetInstance().SetLevel(debug_level::ERROR);
	auto con = std::make_shared<boost::asio::io_context>();
	SECTION("UDP basic send/receive", "[UDP][SEND][RECV]"){
		auto com1 = std::make_unique<UDPCommunicator>(con, static_cast<unsigned short>(8080));
		auto com2 = std::make_unique<UDPCommunicator>(con, static_cast<unsigned short>(8081));

		com1->Connect(*con, "localhost", 8081);
		com2->Connect(*con, "localhost", 8080);

		string msg = "";
		string msg1 = "";
		string msg2 = "";

		msg = "asdf";
		REQUIRE_NOTHROW(com1->Send(msg));
		REQUIRE_NOTHROW(com2->Receive());
		REQUIRE_NOTHROW(msg2 = com2->GetMessage());
		CAPTURE(msg, msg2);
		REQUIRE_THAT(msg, Equals( msg2));

		msg = "fdsa";
		REQUIRE_NOTHROW(com2->Send(msg));
		REQUIRE_NOTHROW(com1->Receive());
		REQUIRE_NOTHROW(msg2 = com1->GetMessage());
		CAPTURE(msg, msg2);
		REQUIRE_THAT(msg, Equals(msg2));

		auto com3 = std::make_unique<UDPCommunicator>(con);
		com3->Connect(*con, "localhost", 2020);
		msg = "hello";
		REQUIRE_NOTHROW(com3->Send(msg));
		REQUIRE_NOTHROW(com1->Receive());
		REQUIRE_NOTHROW(com2->Receive());
		msg1 = com1->GetMessage();
		msg2 = com2->GetMessage();
		REQUIRE_THAT(msg1, !Equals(msg));
		CAPTURE(msg1, msg);
		REQUIRE_THAT(msg2, !Equals(msg));
		CAPTURE(msg2, msg);
	}

	SECTION("UDP basic reply", "[UDP][REPLY]"){
		auto com1 = std::make_unique<UDPCommunicator>(con, static_cast<unsigned short>(8080));
		auto com2 = std::make_unique<UDPCommunicator>(con, static_cast<unsigned short>(8081));

		com1->Connect(*con, "localhost", 8081);

		string msg;
		com1->Send("1234");
		com2->Receive();

		msg = com2->GetMessage();
		std::reverse(msg.begin(), msg.end());
		REQUIRE_NOTHROW(com2->Reply(msg));
		com1->Receive();
		string msg1 = com1->GetMessage();
		REQUIRE(msg1 == msg);
		CAPTURE(msg1, msg);
		msg.erase();
		msg1.erase();

		msg = "asdf";
		REQUIRE_NOTHROW(com1->Reply(msg));
		com2->Receive();
		string msg2 = com2->GetMessage();
		CAPTURE(msg, msg2);
		REQUIRE_THAT(msg, Equals(msg2));
		msg.erase();
		msg2.erase();

		string msg3 = "";
		auto com3 = std::make_unique<UDPCommunicator>(con, static_cast<unsigned short>(8083));

		msg = "dvorak";
		com3->Connect(*con, "localhost", 8080);
		com3->Send(msg);
		com1->Receive();
		com2->Receive();
		msg1 = com1->GetMessage();
		msg2 = com2->GetMessage();
		CAPTURE(msg, msg1, msg2);
		REQUIRE_THAT(msg, Equals(msg1));
		REQUIRE_THAT(msg, !Equals(msg2));
		msg.erase();
		msg1.erase();
		msg2.erase();

		std::this_thread::sleep_for(std::chrono::seconds(5));

		msg = "qwerty";
		REQUIRE_NOTHROW(com1->Reply(msg));
		com2->Receive();
		com3->Receive();
		msg2 = com2->GetMessage();
		msg3 = com3->GetMessage();
		CAPTURE(msg, msg1, msg2, msg3);
		REQUIRE_THAT(msg, Equals(msg2));
		REQUIRE_THAT(msg, !Equals(msg3));
	}

	SECTION("UDP multiple agents", "[UDP][SEND][RECV][REPLY][MULTI]"){
		vector<string> alphabet;
		const int SOCK_MAX = 10;
		for(char32_t i = 0; i < 0x10FFFF; i++){
			if((0xD800 <= i && i <= 0xDFFF) || (0xFDD0 <= i && i <= 0xFDEF) || (i == 0xFFFE || i == 0xFFFF)){
				continue;
			}
			string str(1,i);
			alphabet.push_back(str);
		}
		vector<std::unique_ptr<UDPCommunicator>> agents;
		agents.reserve(SOCK_MAX);
		std::random_device rand_dev;
		std::mt19937 eng(rand_dev());
		std::uniform_int_distribution<> dist(0, alphabet.size() - 1);
		for(int i = 0; i < SOCK_MAX; i += 1){
			INFO("Creating socket number " << i);
			agents.emplace_back(std::make_unique<UDPCommunicator>(con, static_cast<unsigned short>(8080 + i)));
		}
		for(int i = 0; i < SOCK_MAX; i += 2){
			INFO("Connecting " << i << "->" << (i+1));
			agents[i]->Connect(*con, "localhost", 8080 + i + 1);
			INFO("Connecting " << (i + 1) << "->" << i);
			agents[i+1]->Connect(*con, "localhost", 8080 + i);
		}
		string msg1;
		string msg2;
		for(int i = 0; i < 800; i += 2){
			size_t len1 = static_cast<size_t>(dist(eng)) % 1024, 
				   len2 = static_cast<size_t>(dist(eng)) % 1024;

			msg1.reserve(len1);
			msg2.reserve(len2);
			for(size_t j = 0; j < len1; j += 1){
				msg1 += alphabet[dist(eng)];
			}
			for(size_t j = 0; j < len2; j += 1){
				msg2 += alphabet[dist(eng)];
			}

			UDPCommunicator * agent1 = agents[i%SOCK_MAX].get();
			UDPCommunicator * agent2 = agents[(i+1)% SOCK_MAX].get();
			agent1->Send(msg1);
			agent2->Receive();
			msg2 = agent2->GetMessage();
			INFO("msg1: " + msg1 + "\tmsg2: " + msg2);
			CAPTURE(msg1, msg2);
			REQUIRE_THAT(msg1, Equals(msg2));
			

			agent2->Send(msg2);
			agent1->Receive();
			msg1 = agent1->GetMessage();
			INFO("msg1: " + msg1 + "\tmsg2: " + msg2);
			CAPTURE(msg1, msg2);
			REQUIRE_THAT(msg1, Equals(msg2));
			msg1.erase();
			msg2.erase();
		}

		for(int i = 0; i < 800; i += 2){
			size_t len1 = static_cast<size_t>(dist(eng)) % 1024, 
				   len2 = static_cast<size_t>(dist(eng)) %1024;

			msg1.reserve(len1);
			msg2.reserve(len2);
			for(size_t j = 0; j < len1; j += 1){
				msg1 += alphabet[dist(eng)];
			}
			for(size_t j = 0; j < len1; j += 1){
				msg2 += alphabet[dist(eng)];
			}

			UDPCommunicator * agent1 = agents[i%SOCK_MAX].get();
			UDPCommunicator * agent2 = agents[(i+1)% SOCK_MAX].get();
			agent1->Send(msg1);
			agent2->Receive();
			msg2 = agent2->GetMessage();
			INFO("msg1: " + msg1 + "\tmsg2: " + msg2);
			CAPTURE(msg1, msg2);
			REQUIRE_THAT(msg1, Equals(msg2));
			
		

			agent2->Send(msg2);
			agent1->Receive();
			msg1 = agent1->GetMessage();
			INFO("msg1: " + msg1 + "\tmsg2: " + msg2);
			CAPTURE(msg1, msg2);
			REQUIRE_THAT(msg1, Equals(msg2));
			msg1.erase();
			msg2.erase();
		}
	}
}

TEST_CASE("Testing TCPCommunicator", "[TCP]"){
	Logger::GetInstance().SetLevel(debug_level::ERROR);

	auto con1 = std::make_shared<boost::asio::io_context>();
	auto con2 = std::make_shared<boost::asio::io_context>();
	SECTION("TCP basic send/receive", "[TCP][SEND][RECV]"){
		auto server = std::make_unique<TCPCommunicator>(con1, 8080);
		auto client = std::make_unique<TCPCommunicator>(con2);

		
		std::thread client_thread([](boost::asio::io_context &ctx, TCPCommunicator * com){
				com->Connect(ctx, "localhost", 8080);
				}, std::ref(*con2), client.get());
		server->Accept();
		client_thread.join();

		string msg1("asdf");
		string msg2;
		REQUIRE_NOTHROW(client->Send(msg1));
		con2->run();
		REQUIRE_NOTHROW(server->Receive());
		con1->run();

		msg2 = server->GetMessage();
		CAPTURE(msg1, msg2);
		REQUIRE_THAT(msg1, Equals(msg2));

		std::reverse(msg2.begin(), msg2.end());
		REQUIRE_NOTHROW(server->Send(msg2));
		con1->run();
		REQUIRE_NOTHROW(client->Receive());
		con2->run();

		msg1 = client->GetMessage();
		CAPTURE(msg1, msg2);
		REQUIRE_THAT(msg1, Equals(msg2));
	}

	SECTION("TCP reply", "[TCP][REPLY]"){

auto client1 = std::make_unique<TCPCommunicator>(con1);
		auto server1 = std::make_unique<TCPCommunicator>(con2, 8081);


		std::thread t1([&](){
				server1->Accept();
				});

		client1->Connect(*con1, "localhost", 8081);
		t1.join();

		string msg1("asdf");
		string msg2;
		client1->Send(msg1);
		con1->run();
		server1->Receive();
		con2->run();

		msg1 = server1->GetMessage();
		msg2 = "hello";
		REQUIRE_NOTHROW(server1->Reply(msg2));
		con2->run();
		client1->Receive();
		con1->run();
		msg1 = client1->GetMessage();
		CAPTURE(msg1, msg2);
		REQUIRE_THAT(msg1, Equals(msg2));

		msg1 = "goodbye";
		REQUIRE_NOTHROW(client1->Reply(msg1));
		con1->run();
		server1->Receive();
		con2->run();
		msg2 = server1->GetMessage();
		CAPTURE(msg1, msg2);
		REQUIRE_THAT(msg1, Equals(msg2));

		msg1 = "goodbye";
		REQUIRE_NOTHROW(client1->Reply(msg1));
		con1->run();
		server1->Receive();
		con2->run();
		msg2 = server1->GetMessage();
		CAPTURE(msg1, msg2);
		REQUIRE_THAT(msg1, Equals(msg2));
	}

	SECTION("TCP multiple agents", "[TCP][MULTI]"){

		vector<string> alphabet;
		for(char32_t i = 0; i < 0x10FFFF; i++){
			if((0xD800 <= i && i <= 0xDFFF) || (0xFDD0 <= i && i <= 0xFDEF) || (i == 0xFFFE || i == 0xFFFF) || (i == '\0') ){
				continue;
			}
			string str(1,i);
			alphabet.push_back(str);
		}
		vector<std::unique_ptr<TCPCommunicator>> agents;
		agents.reserve(20);
		std::random_device rand_dev;
		std::mt19937 eng(rand_dev());
		std::uniform_int_distribution<> dist(0, alphabet.size() - 1);
		for(int i = 0; i < 20; i += 1){
			if(i % 2 == 0){
				agents.emplace_back(std::make_unique<TCPCommunicator>(con1, 8080 + i));
			} else {
				agents.emplace_back(std::make_unique<TCPCommunicator>(con2, 8080 + i));
			}
		}
		for(int i = 0; i < 20; i += 2){
			agents[i]->Connect(*con1, "localhost", 8080 + i + 1);
			agents[i+1]->Accept();
			con1->run();
			std::thread t1([](boost::asio::io_context &ctx){
					ctx.run();
					}, std::ref(*con2));
			t1.join();

			agents[i+1]->Connect(*con2, "localhost", 8080 + i);
			agents[i]->Accept();
			con1->run();
			std::thread t2([](boost::asio::io_context &ctx){
					ctx.run();
					}, std::ref(*con1));
			t2.join();

		}

		//u32string msg1;
		//u32string msg2;
		string msg1;
		string msg2;
		for(int i = 0; i < 800; i += 2){
			size_t len1 = static_cast<size_t>(dist(eng)) % 1024, 
				   len2 = static_cast<size_t>(dist(eng)) % 1024;

			msg1.reserve(len1);
			msg2.reserve(len2);
			for(size_t j = 0; j < len1; j += 1){
				msg1 += alphabet[dist(eng)];
			}
			for(size_t j = 0; j < len1; j += 1){
				msg2 += alphabet[dist(eng)];
			}

			TCPCommunicator * agent1 = agents[i%20].get();
			TCPCommunicator * agent2 = agents[(i+1)% 20].get();
			agent1->Send(msg1);
			if(i % 2 == 0) con1->run();
			else con2->run();
			agent2->Receive();
			if(i % 2 == 0) con2->run();
			else con1->run();
			msg2 = agent2->GetMessage();
			CAPTURE(msg1, msg2);
			REQUIRE_THAT(msg1, Equals(msg2));
			
			agent2->Send(msg2);
			if(i % 2 == 0) con2->run();
			else con1->run();
			agent1->Receive();
			if(i % 2 == 0) con1->run();
			else con2->run();
			msg1 = agent1->GetMessage();
			REQUIRE_THAT(msg1, Equals(msg2));
			CAPTURE(msg1, msg2);
			msg1.erase();
			msg2.erase();
		}


	}
}

TEST_CASE("Testing DBConnector", "[DB]"){
	Logger::GetInstance().SetLevel(debug_level::ERROR);
	SECTION("DBConnector simple testing with good inputs", "[DB][SIMPLE][GOOD]"){
		DBConnector con("jdbc:mariadb://localhost:3306/FoodServerTest");
		string ingredient("Thyme");
		int grams = 5;
		bool mass = true;

		REQUIRE_NOTHROW(con.CreateIngredient(ingredient, grams, mass));
		ingredient = "Olive Oil";
		grams = 10;
		REQUIRE_NOTHROW(con.CreateIngredient(ingredient, grams, mass));
		REQUIRE_NOTHROW(con.UpdateIngredient(ingredient, -grams));

		ingredient.erase();
		REQUIRE_NOTHROW(ingredient = con.GetIngredientByIndex(0));

		string recipe("Thyme in a bowl");
		string instructions("Add 2 grams of thyme to bowl and stir");
		REQUIRE_NOTHROW(con.CreateRecipe(recipe, instructions));

		float quantity = 2.;
		REQUIRE_NOTHROW(con.MapRecipeToIngredient(recipe, quantity, "Thyme"));
		vector<string> ing_storage;
		vector<string> rec_storage;
		vector<string> ins_storage;

		REQUIRE_NOTHROW(ing_storage = con.GetIngredients(recipe));
		CAPTURE(ing_storage.size());
		REQUIRE(ing_storage.size() == 1);
		CAPTURE(ing_storage[0]);
		REQUIRE_THAT(ing_storage[0], Equals("Thyme"));
		
		REQUIRE_NOTHROW(rec_storage = con.GetRecipes(ing_storage));
		CAPTURE(rec_storage.size());
		REQUIRE(rec_storage.size() == 1);
		CAPTURE(recipe, rec_storage[0]);
		REQUIRE_THAT(recipe, Equals(rec_storage[0]));

		REQUIRE_NOTHROW(ins_storage = con.GetInstructions(rec_storage));
		CAPTURE(ins_storage.size());
		REQUIRE(ins_storage.size() == 1);
		CAPTURE(instructions, ins_storage[0]);
		REQUIRE_THAT(instructions, Equals(ins_storage[0]));

		vector<float> amount = {2.};
		REQUIRE_NOTHROW(con.Reserve(ing_storage, amount));
		REQUIRE_NOTHROW(con.Release(ing_storage, amount));

		REQUIRE_NOTHROW(con.DeleteIngredient(ing_storage[0]));
		REQUIRE_NOTHROW(con.DeleteRecipe(rec_storage[0]));
		REQUIRE_NOTHROW(con.DeleteIngredient(ingredient));

		string ing;
		try{
			ing = con.GetIngredientByIndex(0);
			while(!ing.empty()){
				con.DeleteIngredient(ing);
				ing = con.GetIngredientByIndex(0);
			}
		}
		catch(std::runtime_error re){
		}
	}

	SECTION("DBConnector simple testing with bad inputs", "[DB][SIMPLE][BAD]"){
		DBConnector con("jdbc:mariadb://localhost:3306/FoodServerTest");
		const string good_ing("Thyme");
		const int good_grams = 7;

		const string good_recipe("Thyme in a bowl");
		const string good_instructions("Add 2 grams of thyme to bowl and stir");

		vector<string> good_ing_storage = {good_ing};
		vector<float> good_ing_amount = {2.};
		vector<string> good_rec_storage = {good_recipe};
		vector<string> good_ins_storage = {good_instructions};
		vector<string> bad_ing_storage;
		vector<float> bad_ing_amount;
		vector<string> bad_rec_storage;
		vector<string> bad_ins_storage;
		

		string ingredient("");
		int grams = 5;
		bool mass = true;

		REQUIRE_THROWS(con.CreateIngredient(ingredient, grams, mass));
		ingredient = string(" ");
		REQUIRE_THROWS(con.CreateIngredient(ingredient, grams, mass));

		grams = -4;
		REQUIRE_THROWS(con.CreateIngredient(good_ing, grams, mass));
		string res;
		CAPTURE(res);
		REQUIRE_THROWS(res = con.GetIngredientByIndex(0));
		REQUIRE_THROWS(con.GetIngredientByIndex(20));
		REQUIRE_THROWS(con.GetIngredientByIndex(-1));

		con.CreateIngredient(good_ing, good_grams,  mass);

		REQUIRE_THROWS(con.UpdateIngredient(ingredient, grams));
		REQUIRE_THROWS(con.UpdateIngredient(ingredient, good_grams));
		REQUIRE_THROWS(con.UpdateIngredient(good_ing, grams * 2));

		string bad_recipe("");
		string bad_instructions("asdf");

		REQUIRE_THROWS(con.CreateRecipe(bad_recipe, bad_instructions));
		bad_recipe = string(" ");
		REQUIRE_THROWS(con.CreateRecipe(bad_recipe, bad_instructions));
		bad_instructions = string("");
		REQUIRE_THROWS(con.CreateRecipe(good_recipe, bad_instructions));
		bad_instructions = string(" ");
		REQUIRE_THROWS(con.CreateRecipe(good_recipe, bad_instructions));
		con.CreateRecipe(good_recipe, good_instructions);

		REQUIRE_THROWS(con.MapRecipeToIngredient(good_recipe, good_grams, ingredient));
		ingredient = string("");
		REQUIRE_THROWS(con.MapRecipeToIngredient(good_recipe, good_grams, ingredient));
		REQUIRE_THROWS(con.MapRecipeToIngredient(good_recipe, grams, good_ing));
		REQUIRE_THROWS(con.MapRecipeToIngredient(bad_recipe, good_grams, good_ing));
		bad_recipe = string("");
		REQUIRE_THROWS(con.MapRecipeToIngredient(bad_recipe, good_grams, good_ing));
		con.MapRecipeToIngredient(good_recipe, good_grams, good_ing);

		REQUIRE_THROWS(con.GetIngredients(bad_recipe));
		REQUIRE_THROWS(con.GetRecipes(bad_ing_storage));
		REQUIRE_THROWS(con.GetInstructions(bad_rec_storage));

		REQUIRE_THROWS(con.Reserve(bad_ing_storage, bad_ing_amount));
		REQUIRE_THROWS(con.Reserve(good_ing_storage, bad_ing_amount));
		REQUIRE_THROWS(con.Reserve(bad_ing_storage, good_ing_amount));

		con.Reserve(good_ing_storage, good_ing_amount);

		REQUIRE_THROWS(con.Release(bad_ing_storage, bad_ing_amount));
		REQUIRE_THROWS(con.Release(bad_ing_storage, good_ing_amount));
		REQUIRE_THROWS(con.Release(good_ing_storage, bad_ing_amount));

		con.Release(good_ing_storage, good_ing_amount);

		REQUIRE_THROWS(con.DeleteIngredient(ingredient));
		REQUIRE_THROWS(con.DeleteRecipe(bad_recipe));
		
		con.DeleteIngredient(good_ing);
		con.DeleteRecipe(good_recipe);

		string ing;
		try{
			ing = con.GetIngredientByIndex(0);
			while(!ing.empty()){
				con.DeleteIngredient(ing);
				ing = con.GetIngredientByIndex(0);
			}
		}
		catch(std::runtime_error re){
			Logger::GetInstance().log("[Test] could not delete " + ing + ".\n\t" + re.what(), debug_level::ERROR);
		}
	}

}

TEST_CASE("Testing FileManager", "[FM]"){
	Logger::GetInstance().SetLevel(debug_level::ERROR);
	SECTION("Testing good inputs"){
		FileManager f("pages/index.html");
		REQUIRE(f.getLineCount() == 15);
		std::ifstream file;
		file.open("../pages/index.html");
		file.seekg(0,file.beg);
		string line;
		int i = 0;
		while(std::getline(file, line)){
			CAPTURE(line, f.getLine(i));
			REQUIRE_THAT(line, Equals(f.getLine(i)));
			i += 1;
		}
		file.close();
	}
}

TEST_CASE("Testing MessageManager", "[MM]"){
	Logger::GetInstance().SetLevel(debug_level::ERROR);
	SECTION("Basic MessageManager tests"){
		MessageManager manager;

		int idx = -2;
		REQUIRE_NOTHROW(idx = manager.GetFreeBuffer());
		REQUIRE(idx == 0);
		string message("Hello");
		REQUIRE_NOTHROW(manager.StoreMessage(idx, message));
		string stored;
		REQUIRE_NOTHROW(stored = manager.GetMessageFrom(idx));
		REQUIRE_THAT(stored, Equals(message));
		
		int idx2 = -2;
		REQUIRE_NOTHROW(idx2 = manager.GetFreeBuffer());
		REQUIRE(idx2 != -2);
		string message2("Goodbye");
		REQUIRE_NOTHROW(manager.StoreMessage(idx2, message2));
		string stored2 = "asntd";
		REQUIRE_NOTHROW(stored2 = manager.Pop());
		INFO("[" << idx2 << "]'" << message2 << "' != '" << stored2 << "'");
		REQUIRE_THAT(message2, Equals(stored2));
	}
}


TEST_CASE("NetMessenger basic communication", "[NM]") {
	Logger::GetInstance().SetLevel(debug_level::ERROR);
	NetMessengerFixture fixture;
	
	SECTION("TCP successful message delivery") {
		auto& sender = fixture.createMessenger(tcp, 0);
		auto& receiver = fixture.createMessenger(tcp, 1);
		auto& non_participant = fixture.createMessenger(tcp, 2);
		
		auto recipient = fixture.createRecipient(1);
		std::string test_message = "Hello World";
		
		REQUIRE_NOTHROW(sender.SendTo(test_message, recipient));
		REQUIRE_NOTHROW(receiver.Receive());
		REQUIRE_THROWS_AS(non_participant.Receive(), std::runtime_error);
		
		auto received_message = receiver.GetFirstMessage();
		CAPTURE(received_message, test_message);
		REQUIRE(received_message == test_message);
	}
	
	SECTION("UDP successful message delivery") {
		auto& sender = fixture.createMessenger(udp, 0);
		auto& receiver = fixture.createMessenger(udp, 1);
		
		auto recipient = fixture.createRecipient(1);
		std::string test_message = "Hello World";
		
		REQUIRE_NOTHROW(sender.SendTo(test_message, recipient));
		REQUIRE_NOTHROW(receiver.Receive());
		
		auto received_message = receiver.GetFirstMessage();
		CAPTURE(received_message, test_message);
		REQUIRE(received_message == test_message);
	}
	
	SECTION("Reply functionality TCP") {
		auto& messenger1 = fixture.createMessenger(tcp, 0);
		auto& messenger2 = fixture.createMessenger(tcp, 1);
		auto recipient1 = fixture.createRecipient(0);
		auto recipient2 = fixture.createRecipient(1);
		
		std::string original_message = "Hello";
		std::string reply_message = "Reply";
		
		REQUIRE_NOTHROW(messenger1.SendTo(original_message, recipient1));
		REQUIRE_NOTHROW(messenger2.Receive());
		
		auto received = messenger2.GetFirstMessage();
		CAPTURE(received, original_message);
		REQUIRE(received == original_message);
		
		REQUIRE_NOTHROW(messenger2.ReplyTo(reply_message, recipient1));
		REQUIRE_NOTHROW(messenger1.Receive());
		
		auto reply_received = messenger1.GetFirstMessage();
		CAPTURE(reply_received, reply_message);
		REQUIRE(reply_received == reply_message);
	}
	
	SECTION("Reply functionality UDP") {
		auto& messenger1 = fixture.createMessenger(udp, 0);
		auto& messenger2 = fixture.createMessenger(udp, 1);
		auto recipient1 = fixture.createRecipient(0);
		auto recipient2 = fixture.createRecipient(1);
		
		std::string original_message = "Hello";
		std::string reply_message = "Reply";
		
		REQUIRE_NOTHROW(messenger1.SendTo(original_message, recipient1));
		REQUIRE_NOTHROW(messenger2.Receive());
		
		auto received = messenger2.GetFirstMessage();
		CAPTURE(received, original_message);
		REQUIRE(received == original_message);
		
		REQUIRE_NOTHROW(messenger2.ReplyTo(reply_message, recipient1));
		REQUIRE_NOTHROW(messenger1.Receive());
		
		auto reply_received = messenger1.GetFirstMessage();
		CAPTURE(reply_received, reply_message);
		REQUIRE(reply_received == reply_message);
	}
	
	SECTION("Edge cases") {
		auto& messenger = fixture.createMessenger(tcp, 0);
		
		SECTION("Empty messages") {
			std::string empty_msg = "";
			REQUIRE_NOTHROW(messenger.Send(empty_msg));
		}
		
		SECTION("Large messages") {
			std::string large_msg(10000, 'A');
			REQUIRE_NOTHROW(messenger.Send(large_msg));
		}
		
		SECTION("Unicode messages") {
			std::string unicode_msg = "Hello 世界 🌍";
			REQUIRE_NOTHROW(messenger.Send(unicode_msg));
		}
		
		SECTION("Unicode round-trip") {
			auto& messenger1 = fixture.createMessenger(tcp, 0);
			auto& messenger2 = fixture.createMessenger(tcp, 1);
			auto recipient1 = fixture.createRecipient(0);
			auto recipient2 = fixture.createRecipient(1);
			
			std::vector<std::string> unicode_messages = {
				"Hello 世界 🌍",
				"Привет мир",
				"こんにちは",
				"Ελληνικά",
				"العربية",
				"emoji test 🗺️⭐💯",
				"mixed Ünicode: café naïve résumé"
			};
			
			for(const auto& msg : unicode_messages){
				REQUIRE_NOTHROW(messenger1.SendTo(msg, recipient1));
				REQUIRE_NOTHROW(messenger2.Receive());
				auto received = messenger2.GetFirstMessage();
				CAPTURE(msg, received);
				REQUIRE(received == msg);
			}
		}
		
		SECTION("Special characters") {
			std::string special_msg = "Hello|World\n\t\r";
			REQUIRE_NOTHROW(messenger.Send(special_msg));
		}
	}
}

TEST_CASE("Testing multiple TCP agents", "[NM][TCP][MULTI]"){
		Logger::GetInstance().SetLevel(debug_level::ERROR);
		
		auto io_context = std::make_shared<boost::asio::io_context>();
		
		string alphabet;
		for(int i = 0; i < 256; i += 1){
			if( i == '\0' || i == '\n' || i == '\r' || i == '\t' || i == '|')
				continue;
			alphabet += static_cast<char>(i);
		}
		std::random_device rand_dev;
		std::mt19937 eng(rand_dev());
		std::uniform_int_distribution<> dist(0, alphabet.size() - 1);
		vector<std::unique_ptr<NetMessenger>> agents;
		vector<Recipient> addrs;
		agents.reserve(20);
		addrs.reserve(20);
		for(int i = 0; i < 20; i += 1){
			agents.emplace_back(std::make_unique<NetMessenger>(io_context, tcp));
			agents[i]->Connect(*io_context, "localhost", 8080 + i);
			addrs.emplace_back(std::make_shared<recipient>("localhost", 8080 + i));
		}

		std::thread io_thread([io_context](){
			io_context->run();
		});

		string msg1;
		string msg2;
		for(int i = 0; i < 800; i += 2){
			size_t len1 = static_cast<size_t>(dist(eng)) % 1024, 
				   len2 = static_cast<size_t>(dist(eng)) % 1024;

			msg1.reserve(len1);
			msg2.reserve(len2);
			for(size_t j = 0; j < len1; j += 1){
				msg1 += alphabet[dist(eng)];
			}
			for(size_t j = 0; j < len1; j += 1){
				msg2 += alphabet[dist(eng)];
			}

			int idx1 = i % 20;
			int idx2 = (i + 1) % 20;
			agents[idx1]->SendTo(msg1, addrs[i+1]);
			agents[idx2]->Receive();
			msg2 = agents[idx2]->GetFirstMessage();
			CAPTURE(msg1, msg2);
			REQUIRE_THAT(msg1, Equals(msg2));
			
			agents[idx2]->SendTo(msg2, addrs[i]);
			agents[idx1]->Receive();
			msg1 = agents[idx1]->GetFirstMessage();
			CAPTURE(msg1, msg2);
			REQUIRE_THAT(msg1, Equals(msg2));
			
			agents[idx2]->SendTo(msg2, addrs[i]);
			agents[idx1]->Receive();
			msg1 = agents[idx1]->GetFirstMessage();
			CAPTURE(msg1, msg2);
			REQUIRE_THAT(msg1, Equals(msg2));
			msg1.erase();
			msg2.erase();
		}

		io_thread.join();

		for(int i = 0; i < agents.size(); i += 1){
			agents[i] = nullptr;
		}
	}

TEST_CASE("Testing multiple UDP agents", "[NM][UDP][MULTI]"){
		Logger::GetInstance().SetLevel(debug_level::ERROR);
		
		auto io_context = std::make_shared<boost::asio::io_context>();
		
		vector<string> alphabet;
		for(char32_t i = 0; i < 0x10FFFF; i++){
			if((0xD800 <= i && i <= 0xDFFF) || (0xFDD0 <= i && i <= 0xFDEF) || (i == 0xFFFE || i == 0xFFFF)){
				continue;
			}
			string str(1,i);
			alphabet.push_back(str);
		}
		std::random_device rand_dev;
		std::mt19937 eng(rand_dev());
		std::uniform_int_distribution<> dist(0, alphabet.size() - 1);
		vector<std::unique_ptr<NetMessenger>> agents;
		vector<Recipient> addrs;
			agents.reserve(20);
		addrs.reserve(20);
		for(int i = 0; i < 20; i += 1){
			agents.emplace_back(std::make_unique<NetMessenger>(io_context, udp));
			agents[i]->Connect(*io_context, "localhost", 8080 + i);
			addrs.emplace_back(std::make_shared<recipient>("localhost", 8080 + i));
		}

		std::thread io_thread([io_context](){
			io_context->run();
		});

		//u32string msg1;
		//u32string msg2;
		string msg1;
		string msg2;
		for(int i = 0; i < 800; i += 2){
			size_t len1 = static_cast<size_t>(dist(eng)) % 1024,  
				   len2 = static_cast<size_t>(dist(eng)) % 1024;

			msg1.reserve(len1);
			msg2.reserve(len2);
			for(size_t j = 0; j < len1; j += 1){
				msg1 += alphabet[dist(eng)];
			}
			for(size_t j = 0; j < len1; j += 1){
				msg2 += alphabet[dist(eng)];
			}

			int idx1 = i % 20;
			int idx2 = (i+1)%20;
			agents[idx1]->SendTo(msg1, addrs[i+1]);
			agents[idx2]->Receive();
			msg2 = agents[idx2]->GetFirstMessage();
			CAPTURE(msg1, msg2);
			REQUIRE_THAT(msg1, Equals(msg2));
			
			agents[idx2]->SendTo(msg2, addrs[i]);
			agents[idx1]->Receive();
			msg1 = agents[idx1]->GetFirstMessage();
			CAPTURE(msg1, msg2);
			REQUIRE_THAT(msg1, Equals(msg2));
			msg1.erase();
			msg2.erase();
		}

		io_thread.join();

		for(int i = 0; i < agents.size(); i += 1){
			agents[i] = nullptr;
			addrs[i] = nullptr;
		}
	}

	TEST_CASE("Testing UDP Unicode round-trip", "[NM][UDP][UNICODE]"){
		Logger::GetInstance().SetLevel(debug_level::ERROR);
		
		auto io_context = std::make_shared<boost::asio::io_context>();
		
		auto messenger1 = std::make_unique<NetMessenger>(udp, 0xC001);
		auto messenger2 = std::make_unique<NetMessenger>(udp, 0xC002);
		messenger1->Connect(*io_context, "localhost", 0xC001);
		messenger2->Connect(*io_context, "localhost", 0xC002);
		auto recipient1 = std::make_shared<recipient>("localhost", 0xC001);
		auto recipient2 = std::make_shared<recipient>("localhost", 0xC002);
		
		std::thread io_thread([io_context](){
			io_context->run();
		});
		
		std::vector<std::string> unicode_messages = {
			"Hello 世界 🌍",
			"Привет мир",
			"こんにちは",
			"Ελληνικά",
			"العربية",
			"emoji test 🗺️⭐💯",
			"mixed Ünicode: café naïve résumé",
			"test|with|pipe",
			"multiline\ntest\r\ndata"
		};
		
		for(size_t i = 0; i < unicode_messages.size(); i += 2){
			const auto& msg = unicode_messages[i];
			REQUIRE_NOTHROW(messenger1->SendTo(msg, recipient2));
			REQUIRE_NOTHROW(messenger2->Receive());
			auto received = messenger2->GetFirstMessage();
			CAPTURE(msg, received);
			REQUIRE_THAT(received, Equals( msg ));
		}
		
		io_thread.join();
	}

typedef struct{ string name;
	double count;
	string unit;
} ingredient_request;

typedef struct{
	string name;
	string instruction;
} recipe_request;

string reqToString(ingredient_request req){
	return string(req.name + "|" + std::to_string(req.count) + "|" + req.unit);
}

string reqToString(recipe_request req){
	return string(req.name + "|" + req.instruction);
}


struct riPair {
	double amount;
	string name;
};

TEST_CASE("Testing Server", "[SRV]"){
	Logger::GetInstance().SetLevel(debug_level::ERROR);
	SECTION("Testing syn", "[TCP][UDP][SYN]"){
		boost::asio::io_context dummy_con;
		std::unique_ptr<IDB> db1 = std::make_unique<DBConnector>("jdbc:mariadb://localhost:3306/FoodServerTest");
		auto messenger1 = std::make_unique<NetMessenger>(udp, 2021);
		auto server1 = std::make_unique<Server>(*messenger1, std::move(db1));

		std::unique_ptr<IDB> db2 = std::make_unique<DBConnector>("jdbc:mariadb://localhost:3306/FoodServerTest");
		auto messenger2 = std::make_unique<NetMessenger>(tcp, 2022);
		auto server2 = std::make_unique<Server>(*messenger2, std::move(db2));

		Recipient udp_rec = std::make_shared<recipient>();
		udp_rec->address = "localhost";
		udp_rec->port = 2021;

		Recipient tcp_rec = std::make_shared<recipient>();
		tcp_rec->address = "localhost";
		tcp_rec->port = 2022;

		auto udp_com = std::make_unique<NetMessenger>(udp);
		auto tcp_com = std::make_unique<NetMessenger>(tcp);

		std::thread server1_thread([&server1](){
			server1->Listen();
		});
		udp_com->Connect(dummy_con, udp_rec->address, udp_rec->port);
		udp_com->Send("0");
		udp_com->Receive(false);

		string res = udp_com->GetFirstMessage();

		REQUIRE_THAT(res, Equals("OK"));
		server1->Stop();
		CAPTURE(res);
	

		std::thread server2_thread([&server2](){
			server2->Listen();
		});
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		tcp_com->SendTo("0", tcp_rec);
		tcp_com->Receive(false);

		res = tcp_com->GetFirstMessage();
		CAPTURE(res);
		REQUIRE_THAT(res, Equals("OK"));

		server2->Stop();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if(server1_thread.joinable()){  
			server1_thread.join();
		}
		if(server2_thread.joinable()){  
			server2_thread.join();
		}
	}

	SECTION("Testing addition and deletion requests in UDP", "[UDP][ADD][DEL]"){
		NetMessenger msngr(udp, 2021);
		auto server = std::make_unique<Server>(msngr);
		Recipient r_serv = std::make_shared<recipient>(); 
		r_serv->address = "localhost";
		r_serv->port = 2021;
		auto com = std::make_unique<NetMessenger>(udp);
		auto db = std::make_unique<DBConnector>("jdbc:mariadb://localhost:3306/FoodServerTest");
		
		com->SendTo("0", r_serv);
		ingredient_request ing1 = { "a", 2.0, "g" };
		recipe_request rec1 = { "r", "a" };

		com->Send("10|" + reqToString(ing1));
		com->Receive();
		string response = com->GetFirstMessage();
		CAPTURE(response);
		REQUIRE_THAT("OK", Equals(response));

		string result;
		REQUIRE_NOTHROW(result = db->GetIngredientByIndex(0));
		CAPTURE(result, ing1.name);
		REQUIRE_THAT(result, Equals(ing1.name));

		com->Send("20" + reqToString(ing1));
		com->Receive();
		response = com->GetFirstMessage();
		CAPTURE(response);
		REQUIRE_THAT(response, Equals("OK"));
		REQUIRE_NOTHROW(result = db->GetIngredientByIndex(0));
		CAPTURE(result);
		REQUIRE(result.empty());

		com->Send("11|" + reqToString(rec1));
		com->Receive();
		response = com->GetFirstMessage();
		CAPTURE(response);
		REQUIRE_THAT(response, Equals("OK"));

		server->Stop();
	}

	SECTION("Testing addition and deletion requests in TCP", "[TCP][ADD][DEL]"){
		NetMessenger msngr(tcp, 2021);
		auto server = std::make_unique<Server>(msngr);
		Recipient r_serv = std::make_shared<recipient>(); 
		r_serv->address = "localhost";
		r_serv->port = 2021;
		auto com = std::make_unique<NetMessenger>(tcp);
		auto db = std::make_unique<DBConnector>("jdbc:mariadb://localhost:3306/FoodServerTest");

		com->SendTo("0", r_serv);
		ingredient_request ing1 = { "a", 2.0, "g" };
		recipe_request rec1 = { "r", "a" };

		com->Send("10|" + reqToString(ing1));
		com->Receive();
		string response = com->GetFirstMessage();
		CAPTURE(response);
		REQUIRE_THAT("OK", Equals(response));

		string result;
		REQUIRE_NOTHROW(result = db->GetIngredientByIndex(0));
		CAPTURE(result, ing1.name);
		REQUIRE_THAT(result, Equals(ing1.name));

		com->Send("20" + reqToString(ing1));
		com->Receive();
		response = com->GetFirstMessage();
		CAPTURE(response);
		REQUIRE_THAT(response, Equals("OK"));
		REQUIRE_NOTHROW(result = db->GetIngredientByIndex(0));
		CAPTURE(result);
		REQUIRE(result.empty());

		com->Send("11|" + reqToString(rec1));
		com->Receive();
		response = com->GetFirstMessage();
		CAPTURE(response);
		REQUIRE_THAT(response, Equals("OK"));
		server->Stop();
	}

	SECTION("Testing  reserving, and releasing in UDP", "[UDP][LIN][RES]"){
		Recipient r_serv = std::make_shared<recipient>(); 
		r_serv->address = "localhost";
		r_serv->port = 2021;
		auto com = std::make_unique<NetMessenger>(udp);
		auto db = std::make_unique<DBConnector>("jdbc:mariadb://localhost:3306/FoodServerTest");
		NetMessenger msngr(udp, 2021);
		auto server = std::make_unique<Server>(msngr);

		com->SendTo("0", r_serv);

		ingredient_request ing1 = { "a", 2.0, "g" };
		recipe_request rec1 = { "r", "a" };

		com->Send("10|" + reqToString(ing1));
		com->Receive();
		string response = com->GetFirstMessage();
		com->SendTo("11|" + reqToString(rec1), r_serv);
		com->Receive();
		response = com->GetFirstMessage();

		com->Send("1|a|1.5|10s");
		com->Receive();
		response = com->GetFirstMessage();
		sleep(10);
		com->Send("1|a|0.25|30s");
		com->Receive();
		response = com->GetFirstMessage();
		com->Send("2|a|0.25");
		com->Receive();
		response = com->GetFirstMessage();
		server->Stop();
	}

	SECTION("Testing matchings in UDP", "[UDP][MTC]"){
		NetMessenger msngr(udp, 2021);
		auto server = std::make_unique<Server>(msngr);
		Recipient r_serv = std::make_shared<recipient>();
		r_serv->address = "localhost";
		r_serv->port = 2021;
		auto com = std::make_unique<NetMessenger>(udp);

		com->SendTo("0", r_serv);
		// Method: generate m random recipes, with a random number ('unique' to
		// each recipe), n, of ingredients, then match off of those circumstances.
		//	Recipe generation: create a random title, and a random set of ingredients
		//	by selecting from a list of valid ingredients (see below).
		//	Ingredient generation: generate a random percentile value and if it's
		//	less than 20, don't use the ingredient in any recipe (keep track of
		//	these seperately) otherwise allow it to be used. Generate a random
		//	double for the units, and treat all values as grams.
		

		vector<string> alphabet;
		for(char32_t i = 0; i < 0x10FFFF; i++){
			if((0xD800 <= i && i <= 0xDFFF) || (0xFDD0 <= i && i <= 0xFDEF) || (i == 0xFFFE || i == 0xFFFF)){
				continue;
			}
			string str(1,i);
			alphabet.push_back(str);
		}
		vector<riPair> ing_avail;
		vector<riPair> ing_standalone;
		vector<string> recs;
		vector<vector<riPair>> recipe_ings;
		
		std::random_device rand_dev;
		std::mt19937 eng(rand_dev());
		std::uniform_int_distribution<> dist(0, alphabet.size() - 1);

		int ing_count = 5 + dist(eng);
		int rec_count = 5 + dist(eng);
		for(int i = 0; i < ing_count; i += 1){
			int ing_len = 5 + dist(eng);
			string cur_ing;
			for(int j = 0; j < ing_len; j += 1){
				cur_ing += alphabet[dist(eng)];
			}
			int chance = dist(eng);
			double amount = (double)dist(eng);
			riPair ing = {amount, cur_ing};
			if(chance <= alphabet.size() * 20){
				ing_standalone.push_back(ing);
			} else {
				ing_avail.push_back(ing);
			}
			com->Send("10|" + ing.name + "|" + std::to_string(ing.amount) + "|g" );
			com->Receive();
			string dump = com->GetFirstMessage();
		}

		for(int i = 0; i < rec_count; i += 1){
			int rec_len = 5 + dist(eng);
			string rec_name;
			for(int j = 0; j < rec_len; j += 1){
				rec_name += alphabet[dist(eng)];
			} 
			recs.push_back(rec_name);
			vector<riPair> ings;
			for(int j = 0; j < ing_avail.size(); j += 1){
				if(20 >= dist(eng)){
					ings.push_back(ing_avail[j]);
				}
			}
			recipe_ings.push_back(ings);
			string instructions = "Add " + std::to_string( (1.0  + (double)dist(eng))/((0.5 + (double)dist(eng))) ) + " grams of " + ings[0].name + " to a bowl.\n";
			for(int i = 1; i < ings.size(); i += 1){
				instructions += "Add " + std::to_string( (1.0  + (double)dist(eng))/((0.5 + (double)dist(eng))) ) + " grams of " + ings[i].name + " to a bowl and mix thoroughly.\n";
			}
			instructions += "Bake at 200 celcius for 20 minutes.\n";
			com->Send("11|" + rec_name + "|" + instructions );
			com->Receive();
			string dump = com->GetFirstMessage();
		}


		// Check ingredient->recipe mathching
		for(int i = 0; i < recs.size(); i += 1){
			string recipe = recs[i];

			string ingredients = recipe_ings[i][0].name;
			for(int j = j; j < recipe_ings[i].size(); j += 1){
				ingredients += "|" + recipe_ings[i][j].name;
			}
			com->Send("6|" + ingredients);
			com->Receive();
			string res = com->GetFirstMessage();
			CAPTURE(recipe, res);
			REQUIRE_THAT(recipe, Equals(res));
		}


		// Check recipe->ingredient/instruction matching
		// TODO: get union of selected recipe ingredient sets, strip the 
		// insttructions from the response, and check each ingredient in the 
		// response for membership in the ingredient union set.
		string request = "5";
		vector<string> recipe_subset;
		for(int i = 0; i < recs.size(); i += 1){
			if(dist(eng) <= 20){
				string rec = recs[i];
				request += "|" + rec;
				recipe_subset.push_back(rec);
			}
		}
		com->Send(request);
		com->Receive();
		string res = com->GetFirstMessage();
		vector<string> tokens;
		int pos = 0;
		int head = 0;
		string delim = "\n";
		while((pos = res.find(delim, head)) != res.npos){
			string sub = res.substr(head, pos);
			head = pos + delim.size();
			tokens.push_back(sub);
		}

		vector<string> union_set;
		for(int i = 0; i < recipe_subset.size(); i += 1){
			int idx = std::find(recs.begin(), recs.end(), recipe_subset[i]) - recs.begin();
			for(int j = 0; j < recipe_ings[idx].size(); j += 1){
				if( std::find(union_set.begin(), union_set.end(), recipe_ings[idx][j].name) == union_set.end() ){
					union_set.push_back(recipe_ings[idx][j].name);
				}
			}
		}

		for(int i = 0; i < tokens.size() / 2; i += 1){
			CAPTURE(tokens[i], union_set);
			REQUIRE(std::find(union_set.begin(), union_set.end(), tokens[i]) != union_set.end());
		}
		server->Stop();
	}

}

 
