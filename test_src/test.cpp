#include "../NetMessenger.hpp"
#include "../UDPCommunicator.hpp"
#include "../TCPCommunicator.hpp"
#include "../dbConnector.hpp"
#include "../FileManager.hpp"
#include "../MessageManager.hpp"
#include "../Server.hpp"
#include "../Enums.hpp"

#include <boost/asio/io_context.hpp>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <memory>
#include <string>
#include <random>
#include <iostream>
#include <bits/stdc++.h>
#include <thread>
#include <bits/stdc++.h>
#include <boost/algorithm/string/trim.hpp>

using boost::algorithm::trim;
using std::string;
using std::u32string;
using std::vector;

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

bool stringEqual(u32string str1, u32string str2){
	u32string one(str1);
	u32string two(str2);
	trim(one);
	trim(two);
	if(one.size() != two.size()){
		return false;
	}
	bool equal = true;
	int i = 0;
	while(equal && i < one.size()){
		equal = equal && (one[i] == two[i]);
		i += 1;
	}
	return equal;
}

bool stringEqual(string str1, string str2){
	string one(str1);
	string two(str2);
	trim(one);
	trim(two);
	if(one.size() != two.size()){
		return false;
	}
	bool equal = true;
	int i = 0;
	while(equal && i < one.size()){
		equal = equal && (one[i] == two[i]);
		i += 1;
	}
	return equal;
}

TEST_CASE("Testing Constructors"){
	auto con = std::make_shared<boost::asio::io_context>();
	SECTION("Testing UDPCommunicator","[UDP][Constructor]"){
		con->run();

		CHECK_NOTHROW(std::make_unique<UDPCommunicator>(con, static_cast<unsigned short>(8081)));

		CHECK_NOTHROW(std::make_unique<UDPCommunicator>(con, "127.0.0.1", 8083));

		std::cout << "Moving on from nothrows to throws.\n";
		auto com3 = std::make_unique<UDPCommunicator>(con);
		CHECK_THROWS((std::make_unique<UDPCommunicator>(con)));
		CHECK_THROWS((std::make_unique<UDPCommunicator>(con, static_cast<unsigned short>(0xBEEF))));
		CHECK_THROWS((std::make_unique<UDPCommunicator>(con, "0.0.0.0", 0xBEEF)));
		CHECK_THROWS((std::make_unique<UDPCommunicator>(con, "localhost", 8080)));
	}

	SECTION("Testing TCPCommunicator","[TCP][Constructor]"){
		TCPCommunicator com1(con);

		CHECK_NOTHROW(com1 = TCPCommunicator(con));

		CHECK_NOTHROW(com1 = TCPCommunicator(con, 8080));
		CHECK_NOTHROW(std::make_unique<TCPCommunicator>(con, 8081));

		CHECK_NOTHROW(com1 = TCPCommunicator(con, "127.0.0.1", 8082));
		CHECK_NOTHROW(std::make_unique<TCPCommunicator>(con, "127.0.0.1", 8083));
	}

	SECTION("Testing DBConnector", "[DB][Construtor]"){

		DBConnector db;
		CHECK_NOTHROW(db = DBConnector("jdbc:mariadb://localhost:3306/FoodServerTest") );
		CHECK_THROWS(db = DBConnector("hello"));
	}

	SECTION("Testing MessageManager", "[Storage][Constructor]"){
		MessageManager msg;
		CHECK_NOTHROW(msg = MessageManager());
	}

	SECTION("Testing FileManager", "[FileManager][Constructor]"){
		CHECK_NOTHROW(std::make_unique<FileManager>("test_src/test.txt"));
		CHECK_THROWS((std::make_unique<FileManager>("astd")));
		CHECK_NOTHROW(std::make_unique<FileManager>("test_src/test.txt"));
		auto f2 = std::make_unique<FileManager>("test_src/test.txt");
	}

	SECTION("Testing NetMessenger", "[NetMessenger][Construtor]"){
		NetMessenger nm;
		CHECK_NOTHROW(nm = NetMessenger(udp));
		CHECK_NOTHROW(nm = NetMessenger(tcp));
		CHECK_NOTHROW(nm = NetMessenger(udp, 8080));
		CHECK_NOTHROW(nm = NetMessenger(tcp, 8081));
		CHECK_NOTHROW(nm = NetMessenger( udp, "0.0.0.0", 8082) );
		CHECK_NOTHROW(nm = NetMessenger(tcp, "0.0.0.0", 8083));
		CHECK_NOTHROW( nm = NetMessenger() );
		CHECK_NOTHROW(std::make_unique<NetMessenger>(udp));
		CHECK_THROWS(nm = NetMessenger(udp));

		CHECK_NOTHROW(std::make_unique<NetMessenger>(tcp));
		CHECK_THROWS(nm = NetMessenger(tcp));

		CHECK_NOTHROW(std::make_unique<NetMessenger>(udp));
		CHECK_THROWS(nm = NetMessenger(udp, 0xDEAD));
		CHECK_THROWS(nm = NetMessenger(udp, "0.0.0.0", 0xDEAD));

		CHECK_THROWS(nm = NetMessenger(udp, "hello", 2021));
		CHECK_THROWS(nm = NetMessenger(tcp, "hello", 2021));
	}

	SECTION("Testing Server", "[Server],[Construtor]"){
		Server s;
		DBConnector db;
		auto udp_c = std::make_unique<NetMessenger>(udp, 8080);
		auto tcp_c = std::make_unique<NetMessenger>(tcp, 8080);
		CHECK_NOTHROW(s = Server());
		CHECK_NOTHROW(s = Server());
		CHECK_NOTHROW(s = Server(*udp_c));
		CHECK_NOTHROW(s = Server(*tcp_c));

		CHECK_NOTHROW(std::make_unique<Server>(*udp_c));
		CHECK_NOTHROW(s = Server(*udp_c));

		CHECK_NOTHROW(std::make_unique<Server>(*tcp_c));
		CHECK_NOTHROW(s = Server(*tcp_c));
	}

}

TEST_CASE("Testing connection establishment"){
	SECTION("Testing UDPCommunicator", "[UDP][connect]"){
		auto con = std::make_shared<boost::asio::io_context>();
		con->run();
		UDPCommunicator comm1(con, static_cast<unsigned short>(8080));
		UDPCommunicator comm2(con, static_cast<unsigned short>(8081));

		CHECK_NOTHROW(comm1.Connect(*con, "localhost", 8081));
		CHECK(comm1.remote_address() == "127.0.0.1");
		CHECK(comm1.remote_port() == 8081);

		CHECK_THROWS(comm1.Connect(*con, "hostlocal"));
		CHECK_THROWS(comm1.Connect(*con, "hostlocal", 1234));
		CHECK_THROWS(comm1.Connect(*con, "localhost", -1));
	}


	SECTION("Testing TCPCommunicator", "[TCP][connect]"){
		auto con1 = std::make_shared<boost::asio::io_context>();
		auto con2 = std::make_shared<boost::asio::io_context>();
		auto comm1 = std::make_unique<TCPCommunicator>(con1);
		auto comm2 = std::make_unique<TCPCommunicator>(con2, 8081);

		CHECK_NOTHROW(comm1->Connect(*con1, "localhost", 8081));
		CHECK_NOTHROW(comm2->Accept());
		CHECK(comm1->remote_address() == "127.0.0.1");
		CHECK(comm1->remote_port() == 8081);
	}
}

TEST_CASE("Testing UDPCommunicator", "[UDP]"){
	auto con = std::make_shared<boost::asio::io_context>();
	SECTION("UDP basic send/receive", "[UDP][SEND][RECV]"){
		auto com1 = std::make_unique<UDPCommunicator>(con, static_cast<unsigned short>(8080));
		auto com2 = std::make_unique<UDPCommunicator>(con, static_cast<unsigned short>(8081));

		com1->Connect(*con, "localhost", 8081);
		com2->Connect(*con, "localhost", 8080);

		con->run();
		string msg = "";
		string msg1 = "";
		string msg2 = "";

		msg = "asdf";
		CHECK_NOTHROW(com1->Send(msg));
		CHECK_NOTHROW(com2->Receive());
		CHECK_NOTHROW(msg2 = com2->GetMessage());
		CHECK(stringEqual(msg, msg2) == true);
		CAPTURE(msg, msg2);

		msg = "fdsa";
		CHECK_NOTHROW(com2->Send(msg));
		CHECK_NOTHROW(com1->Receive());
		CHECK_NOTHROW(msg2 = com1->GetMessage());
		con->run();
		CHECK(stringEqual(msg, msg2));
		CAPTURE(msg, msg2);

		auto com3 = std::make_unique<UDPCommunicator>(con);
		com3->Connect(*con, "localhost", 2020);
		msg = "hello";
		CHECK_NOTHROW(com3->Send(msg));
		CHECK_NOTHROW(com1->Receive());
		CHECK_NOTHROW(com2->Receive());
		msg1 = com1->GetMessage();
		msg2 = com2->GetMessage();
		CHECK(stringEqual(msg1,msg) != true);
		CAPTURE(msg1, msg);
		CHECK(stringEqual(msg2, msg)!= true);
		CAPTURE(msg2, msg);
	}

	SECTION("UDP basic reply", "[UDP][REPLY]"){
		auto com1 = std::make_unique<UDPCommunicator>(con, static_cast<unsigned short>(8080));
		auto com2 = std::make_unique<UDPCommunicator>(con, static_cast<unsigned short>(8081));

		com1->Connect(*con, "localhost", 8081);

		con->run();
		string msg;
		com1->Send("1234");
		com2->Receive();

		CAPTURE(msg);
		msg = com2->GetMessage();
		std::reverse(msg.begin(), msg.end());
		CHECK_NOTHROW(com2->Reply(msg));
		com1->Receive();
		string msg2 = com1->GetMessage();
		CAPTURE(msg2);
		CHECK(msg2 == msg);
		msg.erase();
		msg2.erase();

		msg2 = "asdf";
		CHECK_NOTHROW(com1->Reply(msg2));
		com2->Receive();
		msg = com2->GetMessage();
		CHECK(stringEqual(msg,msg2) == true);
		CAPTURE(msg, msg2);
		msg.erase();
		msg2.erase();

		string msg1 = "";
		CAPTURE(msg1);
		auto com3 = std::make_unique<UDPCommunicator>(con, static_cast<unsigned short>(8083));

		msg = "dvorak";
		com3->Connect(*con, "localhost", 8080);
		com3->Send(msg);
		com1->Receive();
		com2->Receive();
		msg1 = com1->GetMessage();
		msg2 = com2->GetMessage();
		CHECK(stringEqual(msg,msg1) == true);
		CAPTURE(msg, msg1);
		CHECK(stringEqual(msg, msg2) != true);
		CAPTURE(msg, msg2);
		msg.erase();
		msg1.erase();
		msg2.erase();

		msg = "qwerty";
		CHECK_NOTHROW(com1->Reply(msg));
		com2->Receive();
		msg1 = com2->GetMessage();
		com3->Receive();
		msg2 = com3->GetMessage();
		CHECK(stringEqual(msg,msg1) != true);
		CAPTURE(msg, msg1);
		CHECK(stringEqual(msg, msg2) == true);
		CAPTURE(msg, msg2);
	}

	SECTION("UDP multiple agents", "[UDP][SEND][RECV][REPLY][MULTI]"){
		vector<string> alphabet;
		const int SOCK_MAX = 10;
		for(char32_t i = 0; i < 0x10FFFF; i++){
			if((0xDFFF <= i && i <= 0xD800) || (0xFDEF <= i && i <= 0xFDD0) || (i == 0xFFFE)){
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
			size_t len1 = static_cast<size_t>(dist(eng)), 
				   len2 = static_cast<size_t>(dist(eng));

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
			CHECK(stringEqual(msg1, msg2));
			

			agent2->Send(msg2);
			agent1->Receive();
			msg1 = agent1->GetMessage();
			INFO("msg1: " + msg1 + "\tmsg2: " + msg2);
			CHECK(stringEqual(msg1, msg2));
			msg1.erase();
			msg2.erase();
		}

		for(int i = 0; i < 800; i += 2){
			size_t len1 = static_cast<size_t>(dist(eng)), 
				   len2 = static_cast<size_t>(dist(eng));

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
			CHECK(stringEqual(msg1, msg2));
			
		

			agent2->Send(msg2);
			agent1->Receive();
			msg1 = agent1->GetMessage();
			INFO("msg1: " + msg1 + "\tmsg2: " + msg2);
			CHECK(stringEqual(msg1, msg2));
			msg1.erase();
			msg2.erase();
		}
	}
}

TEST_CASE("Testing TCPCommunicator", "[TCP]"){

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
		CHECK_NOTHROW(client->Send(msg1));
		CHECK_NOTHROW(server->Receive());

		msg2 = server->GetMessage();
		CHECK(stringEqual(msg1, msg2) == true);
		CAPTURE(msg1, msg2);
		CAPTURE(msg1, msg2);

		std::reverse(msg2.begin(), msg2.end());
		CHECK_NOTHROW(server->Send(msg2));
		CHECK_NOTHROW(client->Receive());

		msg1 = client->GetMessage();
		CHECK(stringEqual(msg1, msg2) == true);
		CAPTURE(msg1, msg2);
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
		server1->Receive();

		msg1 = server1->GetMessage();
		msg2 = "hello";
		CHECK_NOTHROW(server1->Reply(msg2));
		client1->Receive();
		msg1 = client1->GetMessage();
		CHECK(stringEqual(msg1, msg2) == true);
		CAPTURE(msg1, msg2);

		msg1 = "goodbye";
		CHECK_NOTHROW(client1->Reply(msg1));
		server1->Receive();
		msg2 = server1->GetMessage();
		CHECK(stringEqual(msg1, msg2));
		CAPTURE(msg1, msg2);

		msg1 = "goodbye";
		CHECK_NOTHROW(client1->Reply(msg1));
		server1->Receive();
		msg2 = server1->GetMessage();
		CHECK(stringEqual(msg1, msg2));
		CAPTURE(msg1, msg2);
	}

	SECTION("TCP multiple agents", "[TCP][MULTI]"){

		vector<string> alphabet;
		for(char32_t i = 0; i < 0x10FFFF; i++){
			if((0xDFFF <= i && i <= 0xD800) || (0xFDEF <= i && i <= 0xFDD0) || (i == 0xFFFE)){
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
			size_t len1 = static_cast<size_t>(dist(eng)), 
				   len2 = static_cast<size_t>(dist(eng));

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
			agent2->Receive();
			msg2 = agent2->GetMessage();
			CHECK(stringEqual(msg1, msg2));
			CAPTURE(msg1, msg2);
			
			agent2->Send(msg2);
			agent1->Receive();
			msg1 = agent1->GetMessage();
			CHECK(stringEqual(msg1, msg2));
			CAPTURE(msg1, msg2);
			msg1.erase();
			msg2.erase();
		}


	}
}

TEST_CASE("Testing DBConnector", "[DB]"){
	SECTION("DBConnector simple testing with good inputs", "[DB][SIMPLE][GOOD]"){
		DBConnector con("jdbc:mariadb://localhost:3306/FoodServerTest");
		string ingredient("Thyme");
		int grams = 5;
		bool mass = true;

		CHECK_NOTHROW(con.CreateIngredient(ingredient, grams, mass));
		ingredient = "Olive Oil";
		grams = 10;
		CHECK_NOTHROW(con.CreateIngredient(ingredient, grams, mass));
		CHECK_NOTHROW(con.UpdateIngredient(ingredient, -grams));

		ingredient.erase();
		CHECK_NOTHROW(ingredient = con.GetIngredientByIndex(0));
		std::cout << ingredient << "\n";

		string recipe("Thyme in a bowl");
		string instructions("Add 2 grams of thyme to bowl and stir");
		CHECK_NOTHROW(con.CreateRecipe(recipe, instructions));

		CHECK_NOTHROW(con.MapRecipeToIngredient(recipe, 2, "Thyme"));
		vector<string> ing_storage;
		vector<string> rec_storage;
		vector<string> ins_storage;

		CHECK_NOTHROW(ing_storage = con.GetIngredients(recipe));
		CHECK(ing_storage.size() == 2);
		CHECK(stringEqual(ing_storage[0], "Thyme") == true);
		CAPTURE(ing_storage[0]);
		
		CHECK_NOTHROW(rec_storage = con.GetRecipes(ing_storage));
		CHECK(rec_storage.size() == 1);
		CHECK(stringEqual(recipe, rec_storage[9]));
		CAPTURE(recipe, rec_storage[9]);

		CHECK_NOTHROW(ins_storage = con.GetInstructions(rec_storage));
		CHECK(ins_storage.size() == 1);
		CHECK(stringEqual(instructions, ins_storage[0]));
		CAPTURE(instructions, ins_storage[0]);

		vector<float> amount = {2.};
		CHECK_NOTHROW(con.Reserve(ing_storage, amount));
		CHECK_NOTHROW(con.Release(ing_storage, amount));

		CHECK_NOTHROW(con.DeleteIngredient(ing_storage[0]));
		CHECK_NOTHROW(con.DeleteRecipe(rec_storage[0]));
		CHECK_NOTHROW(con.DeleteIngredient(ingredient));

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

		CHECK_THROWS(con.CreateIngredient(ingredient, grams, mass));
		ingredient = string(" ");
		CHECK_THROWS(con.CreateIngredient(ingredient, grams, mass));

		grams = -4;
		CHECK_THROWS(con.CreateIngredient(good_ing, grams, mass));

		CHECK_THROWS(con.GetIngredientByIndex(0));
		CHECK_THROWS(con.GetIngredientByIndex(20));
		CHECK_THROWS(con.GetIngredientByIndex(-1));

		con.CreateIngredient(good_ing, good_grams,  mass);

		CHECK_THROWS(con.UpdateIngredient(ingredient, grams));
		CHECK_THROWS(con.UpdateIngredient(ingredient, good_grams));
		CHECK_THROWS(con.UpdateIngredient(good_ing, grams * 2));

		string bad_recipe("");
		string bad_instructions("asdf");

		CHECK_THROWS(con.CreateRecipe(bad_recipe, bad_instructions));
		bad_recipe = string(" ");
		CHECK_THROWS(con.CreateRecipe(bad_recipe, bad_instructions));
		bad_instructions = string("");
		CHECK_THROWS(con.CreateRecipe(good_recipe, bad_instructions));
		bad_instructions = string(" ");
		CHECK_THROWS(con.CreateRecipe(good_recipe, bad_instructions));
		con.CreateRecipe(good_recipe, good_instructions);

		CHECK_THROWS(con.MapRecipeToIngredient(good_recipe, good_grams, ingredient));
		ingredient = string("");
		CHECK_THROWS(con.MapRecipeToIngredient(good_recipe, good_grams, ingredient));
		CHECK_THROWS(con.MapRecipeToIngredient(good_recipe, grams, good_ing));
		CHECK_THROWS(con.MapRecipeToIngredient(bad_recipe, good_grams, good_ing));
		bad_recipe = string("");
		CHECK_THROWS(con.MapRecipeToIngredient(bad_recipe, good_grams, good_ing));
		con.MapRecipeToIngredient(good_recipe, good_grams, good_ing);

		CHECK_THROWS(con.GetIngredients(bad_recipe));
		CHECK_THROWS(con.GetRecipes(bad_ing_storage));
		CHECK_THROWS(con.GetInstructions(bad_rec_storage));

		CHECK_THROWS(con.Reserve(bad_ing_storage, bad_ing_amount));
		CHECK_THROWS(con.Reserve(good_ing_storage, bad_ing_amount));
		CHECK_THROWS(con.Reserve(bad_ing_storage, good_ing_amount));

		con.Reserve(good_ing_storage, good_ing_amount);

		CHECK_THROWS(con.Release(bad_ing_storage, bad_ing_amount));
		CHECK_THROWS(con.Release(bad_ing_storage, good_ing_amount));
		CHECK_THROWS(con.Release(good_ing_storage, bad_ing_amount));

		con.Release(good_ing_storage, good_ing_amount);

		CHECK_THROWS(con.DeleteIngredient(ingredient));
		CHECK_THROWS(con.DeleteRecipe(bad_recipe));
		
		con.DeleteIngredient(good_ing);
		con.DeleteRecipe(good_recipe);

	}

	SECTION("DBConnector testing against SQL injection", "[DB][INJECTION]"){
		DBConnector con("jdbc:mariadb://localhost:3306/FoodServerTest");
		string ingredient("; show tables;");
		string recipe("; describe groceries;");
		string instruction("; describe recipes");
		float grams = 1.0;
		vector<string> ing_sto = {ingredient};
		vector<string> rec_sto = {recipe};
		vector<string> ins_sto = {instruction};
		vector<float> amt_sto = {grams};


		CHECK_THROWS( con.CreateIngredient(ingredient, grams, true) );
		CHECK_THROWS( con.CreateRecipe(recipe, instruction) );
		CHECK_THROWS(con.MapRecipeToIngredient(recipe, grams, ingredient));
		CHECK_THROWS(con.DeleteIngredient(ingredient));
		CHECK_THROWS(con.DeleteRecipe(recipe));
		CHECK_THROWS(con.Reserve(ing_sto, amt_sto));

		CHECK_THROWS(con.UpdateIngredient(ingredient, grams));
		CHECK_THROWS(con.GetIngredients(recipe));
		CHECK_THROWS(con.GetRecipes(ing_sto));
		CHECK_THROWS(con.GetInstructions(rec_sto));

		CHECK_THROWS(con.Release(ing_sto, amt_sto));
	}

}

TEST_CASE("Testing FileManager", "[FM]"){
	SECTION("Testing good inputs"){
		FileManager f("../pages/index.html");
		CHECK(f.getLineCount() == 15);
		std::ifstream file;
		file.open("../pages/index.html");
		file.seekg(0,file.beg);
		string line;
		int i = 0;
		while(std::getline(file, line)){
			INFO("i=" << i << "; line='" << line << "'; file line = '" << f.getLine(i) << "'");
			CHECK(stringEqual(line, f.getLine(i)));
			i += 1;
		}
		file.close();
	}
}

TEST_CASE("Testing MessageManager", "[MM]"){
	SECTION("Basic MessageManager tests"){
		MessageManager manager;

		int idx = -2;
		CHECK_NOTHROW(idx = manager.GetFreeBuffer());
		CHECK(idx == 0);
		string message("Hello");
		CHECK_NOTHROW(manager.StoreMessage(idx, message));
		string stored;
		CHECK_NOTHROW(stored = manager.GetMessageFrom(idx));
		CHECK(stringEqual(stored, message) == true);
		CAPTURE(stored, message);
		
		int idx2 = -2;
		CHECK_NOTHROW(idx2 = manager.GetFreeBuffer());
		CHECK(idx2 != -2);
		string message2("Goodbye");
		CHECK_NOTHROW(manager.StoreMessage(idx2, message2));
		string stored2 = "asntd";
		CHECK_NOTHROW(stored = manager.Pop());
		INFO("[" << idx2 << "]'" << message2 << "' != '" << stored2 << "'");
		CHECK(stringEqual(message2, stored2));
		CAPTURE(message2, stored2);
	}
}


TEST_CASE("NetMessenger basic communication", "[NM]") {
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
		REQUIRE(received == original_message);
		
		REQUIRE_NOTHROW(messenger2.ReplyTo(reply_message, recipient1));
		REQUIRE_NOTHROW(messenger1.Receive());
		
		auto reply_received = messenger1.GetFirstMessage();
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
		REQUIRE(received == original_message);
		
		REQUIRE_NOTHROW(messenger2.ReplyTo(reply_message, recipient1));
		REQUIRE_NOTHROW(messenger1.Receive());
		
		auto reply_received = messenger1.GetFirstMessage();
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
		
		SECTION("Special characters") {
			std::string special_msg = "Hello|World\n\t\r";
			REQUIRE_NOTHROW(messenger.Send(special_msg));
		}
	}
}

TEST_CASE("Testing multiple TCP agents", "[NM][TCP][MULTI]"){
		string alphabet;
		for(int i = 0; i < 256; i += 1){
			if(i == '\n' || i == '\r' || i == '\t' || i == '|')
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
			agents.emplace_back(std::make_unique<NetMessenger>(tcp, 8080 + i));
			addrs.emplace_back(std::make_shared<recipient>("localhost", 8080 + i));
		}

		//u32string msg1;
		//u32string msg2;
		string msg1;
		string msg2;
		for(int i = 0; i < 800; i += 2){
			size_t len1 = static_cast<size_t>(dist(eng)), 
				   len2 = static_cast<size_t>(dist(eng));

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
			CHECK(stringEqual(msg1, msg2));
			CAPTURE(msg1, msg2);
			
			agents[idx2]->SendTo(msg2, addrs[i]);
			agents[idx1]->Receive();
			msg1 = agents[idx1]->GetFirstMessage();
			CHECK(stringEqual(msg1, msg2));
			CAPTURE(msg1, msg2);
			
			agents[idx2]->SendTo(msg2, addrs[i]);
			agents[idx1]->Receive();
			msg1 = agents[idx1]->GetFirstMessage();
			CHECK(stringEqual(msg1, msg2));
			msg1.erase();
			msg2.erase();
		}

		for(int i = 0; i < agents.size(); i += 1){
			agents[i] = nullptr;
		}
	}

TEST_CASE("Testing multiple UDP agents", "[NM][UDP][MULTI]"){
		
		vector<string> alphabet;
		for(char32_t i = 0; i < 0x10FFFF; i++){
			if((0xDFFF <= i && i <= 0xD800) || (0xFDEF <= i && i <= 0xFDD0) || (i == 0xFFFE)){
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
			agents.emplace_back(std::make_unique<NetMessenger>(udp, 8080 + i));
			addrs.emplace_back(std::make_shared<recipient>("localhost", 8080 + i));
		}

		//u32string msg1;
		//u32string msg2;
		string msg1;
		string msg2;
		for(int i = 0; i < 800; i += 2){
			size_t len1 = static_cast<size_t>(dist(eng)), 
				   len2 = static_cast<size_t>(dist(eng));

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
			CHECK(stringEqual(msg1, msg2));
			CAPTURE(msg1, msg2);
			
			agents[idx2]->SendTo(msg2, addrs[i]);
			agents[idx1]->Receive();
			msg1 = agents[idx1]->GetFirstMessage();
			CHECK(stringEqual(msg1, msg2));
			CAPTURE(msg1, msg2);
			msg1.erase();
			msg2.erase();
		}

		for(int i = 0; i < agents.size(); i += 1){
			agents[i] = nullptr;
			addrs[i] = nullptr;
		}
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
	SECTION("Testing syn", "[TCP][UDP][SYN]"){
		std::unique_ptr<IDB> db1 = std::make_unique<DBConnector>("jdbc:mariadb://localhost:3306/FoodServerTest");
		auto messenger1 = std::make_unique<NetMessenger>(udp);
		auto server1 = std::make_unique<Server>(*messenger1, std::move(db1));

		std::unique_ptr<IDB> db2 = std::make_unique<DBConnector>("jdbc:mariadb://localhost:3306/FoodServerTest");
		auto messenger2 = std::make_unique<NetMessenger>(tcp);
		auto server2 = std::make_unique<Server>(*messenger2, std::move(db2));

		Recipient udp_rec = std::make_shared<recipient>();
		udp_rec->address = "localhost";
		udp_rec->port = 2021;

		Recipient tcp_rec = std::make_shared<recipient>();
		tcp_rec->address = "localhost";
		tcp_rec->port = 2022;

		auto udp_com = std::make_unique<NetMessenger>(udp);
		auto tcp_com = std::make_unique<NetMessenger>(tcp);

		udp_com->SendTo("0", udp_rec);
		udp_com->Receive();

		CHECK(stringEqual(udp_com->GetFirstMessage(), "OK"));
		CAPTURE(udp_com->GetFirstMessage());

		tcp_com->SendTo("0", tcp_rec);
		tcp_com->Receive();

		CHECK(stringEqual(tcp_com->GetFirstMessage(), "OK"));
		CAPTURE(tcp_com->GetFirstMessage());
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
		CHECK(stringEqual("OK", response));
		CAPTURE(response);

		string result;
		CHECK_NOTHROW(result = db->GetIngredientByIndex(0));
		CHECK(stringEqual(result, ing1.name));
		CAPTURE(result, ing1.name);

		com->Send("20" + reqToString(ing1));
		com->Receive();
		response = com->GetFirstMessage();
		CHECK(stringEqual(response, "OK"));
		CAPTURE(response);
		CHECK_NOTHROW(result = db->GetIngredientByIndex(0));
		CHECK(result.empty());

		com->Send("11|" + reqToString(rec1));
		com->Receive();
		response = com->GetFirstMessage();
		CHECK(stringEqual(response, "OK"));
		CAPTURE(response);
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
		CHECK(stringEqual("OK", response));
		CAPTURE(response);

		string result;
		CHECK_NOTHROW(result = db->GetIngredientByIndex(0));
		CHECK(stringEqual(result, ing1.name));
		CAPTURE(result, ing1.name);

		com->Send("20" + reqToString(ing1));
		com->Receive();
		response = com->GetFirstMessage();
		CHECK(stringEqual(response, "OK"));
		CAPTURE(response);
		CHECK_NOTHROW(result = db->GetIngredientByIndex(0));
		CHECK(result.empty());

		com->Send("11|" + reqToString(rec1));
		com->Receive();
		response = com->GetFirstMessage();
		CHECK(stringEqual(response, "OK"));
		CAPTURE(response);
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
			if((0xDFFF <= i && i <= 0xD800) || (0xFDEF <= i && i <= 0xFDD0) || (i == 0xFFFE)){
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

		for(int i = 0; rec_count; i += 1){
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
			CHECK(stringEqual(recipe, res));
			CAPTURE(recipe, res);
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
			CHECK(std::find(union_set.begin(), union_set.end(), tokens[i]) != union_set.end());
		}


	}

}

 
