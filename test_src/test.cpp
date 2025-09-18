#include "../NetMessenger.hpp"
#include "../UDPCommunicator.hpp"
#include "../TCPCommunicator.hpp"
#include "../dbConnector.hpp"
#include "../FileManager.hpp"
#include "../MessageManager.hpp"
#include "../Server.hpp"
#include "../Enums.hpp"

#include <boost/asio/io_context.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
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

bool stringEqual(u32string str1, u32string str2){
	u32string one(str1);
	u32string two(str2);
	trim(one);
	trim(two);
	bool equal = false;
	for(int i = 0; i < std::min(str1.size(), str2.size()); i += 1){
		equal = (one[i] == two[i]);
	}
	return equal;
}

bool stringEqual(string str1, string str2){
	string one(str1);
	string two(str2);
	trim(one);
	trim(two);
	if(one.size() == 0 && two.size() == 0){
		return true;
	}
	bool equal = false;
	for(int i = 0; i < std::min(str1.size(), str2.size()); i += 1){
		equal = (one[i] == two[i]);
	}
	return equal;
}

TEST_CASE("Testing Constructors"){
	boost::asio::io_context con;
	SECTION("Testing UDPCommunicator","[UDP][Constructor]"){
		UDPCommunicator com1;
		UDPCommunicator *com2;
		UDPCommunicator *com3;
		con.run();

		CHECK_NOTHROW(com1 = UDPCommunicator(con));

		CHECK_NOTHROW(com1 = UDPCommunicator(con, 8080));
		CHECK_NOTHROW(com2 = new UDPCommunicator(con, 8081));
		delete com2;

		CHECK_NOTHROW(com1 = UDPCommunicator(con, "127.0.0.1", 8082));
		CHECK_NOTHROW(com2 = new UDPCommunicator(con, "127.0.0.1", 8083));
		delete com2;

		std::cout << "Moving on from nothrows to throws.\n";
		com3 = new UDPCommunicator(con);
		com2 = nullptr;
		CHECK_THROWS(com2 = new UDPCommunicator(con));
		CHECK_THROWS(com2 = new UDPCommunicator(con, 0xBEEF));
		CHECK_THROWS(com2 = new UDPCommunicator(con, "0.0.0.0", 0xBEEF));
		CHECK_THROWS(com2 = new UDPCommunicator(con, "localhost", 8080));
		delete(com2);
		delete(com3);
		com2 = nullptr;
		com3 = nullptr;
	}

	SECTION("Testing TCPCommunicator","[TCP][Constructor]"){
		TCPCommunicator com1;
		TCPCommunicator *com2;

		CHECK_NOTHROW(com1 = TCPCommunicator(con));

		CHECK_NOTHROW(com1 = TCPCommunicator(con, 8080));
		CHECK_NOTHROW(com2 = new TCPCommunicator(con, 8081));
		delete(com2);

		CHECK_NOTHROW(com1 = TCPCommunicator(con, "127.0.0.1", 8082));
		CHECK_NOTHROW(com2 = new TCPCommunicator(con, "127.0.0.1", 8083));
		delete(com2);

	}

	SECTION("Testing DBConnector", "[DB][Construtor]"){

		DBConnector db;
		CHECK_NOTHROW(db = DBConnector("jdbc:mariadb://localhost:3306/foodservertest") );
		CHECK_THROWS(db = DBConnector("hello"));
	}

	SECTION("Testing MessageManager", "[Storage][Constructor]"){
		MessageManager msg;
		CHECK_NOTHROW(msg = MessageManager());
	}

	SECTION("Testing FileManager", "[FileManager][Constructor]"){
		FileManager * f;
		CHECK_NOTHROW(f = new FileManager("../test_src/test.txt"));
		delete f;
		f = nullptr;
		CHECK_THROWS(f = new FileManager("astd"));
		CHECK_NOTHROW(f = new FileManager("../test_src/test.txt"));
		FileManager * f2;
		CHECK_NOTHROW(f2 = new FileManager(*f));
		delete f;
		delete f2;
		f = nullptr;
		f2 = nullptr;
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
		NetMessenger * nm2;
		CHECK_NOTHROW(nm2 = new NetMessenger(udp));
		CHECK_THROWS(nm = NetMessenger(udp));
		delete(nm2);

		CHECK_NOTHROW( nm2 = new NetMessenger(tcp) );
		CHECK_THROWS(nm = NetMessenger(tcp));
		delete(nm2);

		CHECK_NOTHROW( nm2 = new NetMessenger(udp) );
		CHECK_THROWS(nm = NetMessenger(udp, 0xDEAD));
		CHECK_THROWS(nm = NetMessenger(udp, "0.0.0.0", 0xDEAD));

		CHECK_THROWS(nm = NetMessenger(udp, "hello", 2021));
		CHECK_THROWS(nm = NetMessenger(tcp, "hello", 2021));
		delete(nm2);
	}

	SECTION("Testing Server", "[Server],[Construtor]"){
		Server s;
		CHECK_NOTHROW(s = Server(udp));
		CHECK_NOTHROW(s = Server(tcp));
		CHECK_NOTHROW(s = Server(udp, 8080));
		CHECK_NOTHROW(s = Server(tcp, 8081));

		Server *s2;
		CHECK_NOTHROW(s2 = new Server(udp));
		CHECK_THROWS(s = Server(udp));

		CHECK_NOTHROW(s2 = new Server(tcp));
		CHECK_THROWS(s = Server(tcp));
		delete s2;

	}

}

TEST_CASE("Testing connection establishment"){
	SECTION("Testing UDPCommunicator", "[UDP][connect]"){
		boost::asio::io_context con;
		con.run();
		UDPCommunicator comm1(con, 8080);
		UDPCommunicator comm2(con, 8081);

		CHECK_NOTHROW(comm1.Connect(con, "localhost", 8081));
		CHECK(comm1.remote_address() == "127.0.0.1");
		CHECK(comm1.remote_port() == 8081);

		CHECK_THROWS(comm1.Connect(con, "hostlocal"));
		CHECK_THROWS(comm1.Connect(con, "hostlocal", 1234));
		CHECK_THROWS(comm1.Connect(con, "localhost", -1));
	}


	SECTION("Testing TCPCommunicator", "[TCP][connect]"){
		boost::asio::io_context con1, con2;
		TCPCommunicator *comm1 = new TCPCommunicator(con1);
		TCPCommunicator *comm2 = new TCPCommunicator(con2, 8081);

		CHECK_NOTHROW(comm1->Connect(con1, "localhost", 8081));
		CHECK_NOTHROW(comm2->Accept());

		std::thread t1([](boost::asio::io_context &ctx){
				ctx.run();
		}, std::ref(con2));
		con1.run();
		t1.join();
		CHECK(comm1->remote_address() == "127.0.0.1");
		CHECK(comm1->remote_port() == 8081);
		delete comm1;
		delete comm2;
	}
}

TEST_CASE("Testing UDPCommunicator", "[UDP]"){
	boost::asio::io_context con;
	SECTION("UDP basic send/receive", "[UDP][SEND][RECV]"){
		UDPCommunicator * com1 = new UDPCommunicator(con, 8080);
		UDPCommunicator * com2 = new UDPCommunicator(con, 8081);

		com1->Connect(con, "localhost", 8081);
		com2->Connect(con, "localhost", 8080);

		con.run();
		string msg = "";
		string msg1 = "";
		string msg2 = "";

		msg = "asdf";
		CHECK_NOTHROW(com1->Send(msg));
		CHECK_NOTHROW(com2->Receive());
		CHECK_NOTHROW(msg2 = com2->GetMessage());
		CHECK(stringEqual(msg, msg2) == true);

		msg = "fdsa";
		CHECK_NOTHROW(com2->Send(msg));
		CHECK_NOTHROW(com1->Receive());
		CHECK_NOTHROW(msg2 = com1->GetMessage());
		con.run();
		CHECK(stringEqual(msg, msg2));

		UDPCommunicator * com3 = new UDPCommunicator(con);
		com3->Connect(con, "localhost", 2020);
		msg = "hello";
		CHECK_NOTHROW(com3->Send(msg));
		CHECK_NOTHROW(com1->Receive());
		CHECK_NOTHROW(com2->Receive());
		msg1 = com1->GetMessage();
		msg2 = com2->GetMessage();
		CHECK(stringEqual(msg1,msg) != true);
		CHECK(stringEqual(msg2, msg)!= true);

		delete(com1);
		delete(com2);
		com1 = nullptr;
		com2 = nullptr;

	}

	SECTION("UDP basic reply", "[UDP][REPLY]"){
		UDPCommunicator * com1 = new UDPCommunicator(con, 8080);
		UDPCommunicator * com2 = new UDPCommunicator(con, 8081);

		com1->Connect(con, "localhost", 8081);

		con.run();
		string msg;
		com1->Send("1234");
		com2->Receive();

		msg = com2->GetMessage();
		std::reverse(msg.begin(), msg.end());
		CHECK_NOTHROW(com2->Reply(msg));
		com1->Receive();
		string msg2 = com1->GetMessage();
		CHECK(msg2 == msg);
		msg.erase();
		msg2.erase();

		msg2 = "asdf";
		CHECK_NOTHROW(com1->Reply(msg2));
		com2->Receive();
		msg = com2->GetMessage();
		CHECK(stringEqual(msg,msg2) == true);
		msg.erase();
		msg2.erase();

		string msg1 = "";
		UDPCommunicator * com3 = new UDPCommunicator(con, 8083);

		msg = "dvorak";
		com3->Connect(con, "localhost", 8080);
		com3->Send(msg);
		com1->Receive();
		com2->Receive();
		msg1 = com1->GetMessage();
		msg2 = com2->GetMessage();
		CHECK(stringEqual(msg,msg1) == true);
		CHECK(stringEqual(msg, msg2) != true);
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
		CHECK(stringEqual(msg, msg2) == true);


		delete(com1);
		com1 = nullptr;
		delete(com2);
		com2 = nullptr;
		delete(com3);
		com3 = nullptr;

	}

	SECTION("UDP multiple agents", "[UDP][SEND][RECV][REPLY][MULTI]"){
		vector<string> alphabet;
		for(char32_t i = 0; i < 0x10FFFF; i++){
			if((0xDFFF <= i && i <= 0xD800) || (0xFDEF <= i && i <= 0xFDD0) || (i == 0xFFFE)){
				continue;
			}
			string str(1,i);
			alphabet.push_back(str);
		}
		vector<UDPCommunicator *> agents;
		std::random_device rand_dev;
		std::mt19937 eng(rand_dev());
		std::uniform_int_distribution<> dist(0, alphabet.size() - 1);
		for(int i = 0; i < 20; i += 1){
			agents[i] = new UDPCommunicator(con, 8080 + i);
		}
		for(int i = 0; i < 20; i += 2){
			agents[i]->Connect(con, "localhost", 8080 + i + 1);
			agents[i+1]->Connect(con, "localhost", 8080 + i);
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
			for(size_t j = 0; j < len1; j += 1){
				msg2 += alphabet[dist(eng)];
			}

			UDPCommunicator * agent1 = agents[i%20];
			UDPCommunicator * agent2 = agents[(i+1)% 20];
			agent1->Send(msg1);
			agent2->Receive();
			msg2 = agent2->GetMessage();
			CHECK(stringEqual(msg1, msg2));
			
			agent2->Send(msg2);
			agent1->Receive();
			msg1 = agent1->GetMessage();
			CHECK(stringEqual(msg1, msg2));
			msg1.erase();
			msg2.erase();
		}

		for(int i = 0; i < agents.size(); i += 1){
			delete(agents[i]);
			agents[i] = nullptr;
		}
	}
}

TEST_CASE("Testing TCPCommunicator", "[TCP]"){

	boost::asio::io_context con1, con2;
	SECTION("TCP basic send/receive", "[TCP][SEND][RECV]"){
		TCPCommunicator * com1 = new TCPCommunicator(con1, 8080);
		TCPCommunicator * com2 = new TCPCommunicator(con2, 8081);


		std::thread t1([](boost::asio::io_context &ctx, TCPCommunicator * com){
				com->Connect(ctx, "localhost", 8081);
				ctx.run();
				}, std::ref(con2), com1);

		com2->Accept();
		t1.join();
		
		std::thread t2([](boost::asio::io_context &ctx, TCPCommunicator * com){
				com->Connect(ctx, "localhost", 8081);
				ctx.run();
				}, std::ref(con1), com2);
		com1->Accept();
		t2.join();

		string msg1("asdf");
		string msg2;
		CHECK_NOTHROW(com1->Send(msg1));
		CHECK_NOTHROW(com2->Receive());

		msg2 = com2->GetMessage();
		CHECK(stringEqual(msg1, msg2) == true);

		std::reverse(msg2.begin(), msg2.end());
		CHECK_NOTHROW(com2->Send(msg2));
		CHECK_NOTHROW(com1->Receive());

		msg1 = com1->GetMessage();
		CHECK(stringEqual(msg1, msg2) == true);

		delete(com1);
		com1 = nullptr;
		delete(com2);
		com2 = nullptr;
		
	}

	SECTION("TCP reply", "[TCP][REPLY]"){

		TCPCommunicator * com1 = new TCPCommunicator(con1, 8080);
		TCPCommunicator * com2 = new TCPCommunicator(con2, 8081);


		com1->Connect(con1, "localhost", 8081);
		com2->Accept();

		std::thread t1([](boost::asio::io_context &ctx){
				ctx.run();
				}, std::ref(con2));

		t1.join();
		com2->Connect(con2, "localhost", 8080);
		com1->Accept();

		std::thread t2([](boost::asio::io_context &ctx){
				ctx.run();
				}, std::ref(con1));

		t2.join();

		string msg1("asdf");
		string msg2;
		com1->Send(msg1);
		com2->Receive();

		msg1 = com2->GetMessage();
		msg2 = "hello";
		CHECK_NOTHROW(com2->Reply(msg2));
		com1->Receive();
		msg1 = com1->GetMessage();
		CHECK(stringEqual(msg1, msg2) == true);

		msg1 = "goodbye";
		CHECK_NOTHROW(com1->Reply(msg1));
		com2->Receive();
		msg2 = com2->GetMessage();
		CHECK(stringEqual(msg1, msg2));
		
		delete(com1);
		com1 = nullptr;
		delete(com2);
		com2 = nullptr;

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
		vector<TCPCommunicator *> agents;
		std::random_device rand_dev;
		std::mt19937 eng(rand_dev());
		std::uniform_int_distribution<> dist(0, alphabet.size() - 1);
		for(int i = 0; i < 20; i += 1){
			if(i % 2 == 0){
				agents[i] = new TCPCommunicator(con1, 8080 + i);
			} else {
				agents[i] = new TCPCommunicator(con2, 8080 + i);
			}
		}
		for(int i = 0; i < 20; i += 2){
			agents[i]->Connect(con1, "localhost", 8080 + i + 1);
			agents[i+1]->Accept();
			con1.run();
			std::thread t1([](boost::asio::io_context &ctx){
					ctx.run();
					}, std::ref(con2));
			t1.join();

			agents[i+1]->Connect(con2, "localhost", 8080 + i);
			agents[i]->Accept();
			con1.run();
			std::thread t2([](boost::asio::io_context &ctx){
					ctx.run();
					}, std::ref(con1));
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

			TCPCommunicator * agent1 = agents[i%20];
			TCPCommunicator * agent2 = agents[(i+1)% 20];
			agent1->Send(msg1);
			agent2->Receive();
			msg2 = agent2->GetMessage();
			CHECK(stringEqual(msg1, msg2));
			
			agent2->Send(msg2);
			agent1->Receive();
			msg1 = agent1->GetMessage();
			CHECK(stringEqual(msg1, msg2));
			msg1.erase();
			msg2.erase();
		}

		for(int i = 0; i < agents.size(); i += 1){
			delete(agents[i]);
			agents[i] = nullptr;
		}
	}
}

TEST_CASE("Testing DBConnector"){
	SECTION("DBConnector simple testing with good inputs", "[DB][SIMPLE][GOOD]"){
		DBConnector con("jdbc:mariadb://localhost:3306/foodservertest");
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
		
		CHECK_NOTHROW(rec_storage = con.GetRecipes(ing_storage));
		CHECK(rec_storage.size() == 1);
		CHECK(stringEqual(recipe, rec_storage[9]));

		CHECK_NOTHROW(ins_storage = con.GetInstructions(rec_storage));
		CHECK(ins_storage.size() == 1);
		CHECK(stringEqual(instructions, ins_storage[0]));

		vector<float> amount = {2.};
		CHECK_NOTHROW(con.Reserve(ing_storage, amount));
		CHECK_NOTHROW(con.Release(ing_storage, amount));

		CHECK_NOTHROW(con.DeleteIngredient(ing_storage[0]));
		CHECK_NOTHROW(con.DeleteRecipe(rec_storage[0]));
		CHECK_NOTHROW(con.DeleteIngredient(ingredient));

	}

	SECTION("DBConnector simple testing with bad inputs", "[DB][SIMPLE][BAD]"){
		DBConnector con("jdbc:mariadb://localhost:3306/foodservertest");
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
		DBConnector con("jdbc:mariadb://localhost:3306/foodservertest");
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
		
		int idx2 = -2;
		CHECK_NOTHROW(idx2 = manager.GetFreeBuffer());
		CHECK(idx2 != -2);
		string message2("Goodbye");
		CHECK_NOTHROW(manager.StoreMessage(idx2, message2));
		string stored2;
		CHECK_NOTHROW(stored = manager.Pop());
		INFO("'" << message2 << "' == '" << stored2 << "'");
		CHECK(stringEqual(message2, stored2));
	}
}


TEST_CASE("Testing NetMessenger", "[NM]"){
	SECTION("Testing basic TCP NetMessenger", "[NM][TCP]"){
		NetMessenger *nm1 = new NetMessenger(tcp, 8080);
		NetMessenger *nm2 = new NetMessenger(tcp, 8081);
		NetMessenger *nm3 = new NetMessenger(tcp, 8083);
		Recipient * r1 = std::make_shared();
		Recipient * r2 = std::make_shared();

		r1->address = "localhost";
		r2->address = "localhost";
		r1->port = 8080;
		r2->port = 8081;

		string msg1("Hello"), msg2, msg3;
		CHECK_NOTHROW( nm1->SendTo(msg1, r1) );
		CHECK_NOTHROW( nm2->Receive() );
		CHECK_NOTHROW( nm3->Receive() );

		CHECK_NOTHROW( msg2 = nm2->GetFirstMessage() );
		CHECK_NOTHROW( msg3 = nm3->GetFirstMessage() );

		CHECK(stringEqual(msg1, msg2));
		CHECK(!stringEqual(msg1, msg3));
		CHECK(!stringEqual(msg2, msg3));

		msg2 = string("Goodbye");
		CHECK_NOTHROW(nm2->ReplyTo(msg2, r1));
		CHECK_NOTHROW(nm1->Receive());
		CHECK_NOTHROW(nm3->Receive());

		CHECK_NOTHROW(msg1 = nm1->GetFirstMessage());
		CHECK_NOTHROW(msg3 = nm3->GetFirstMessage());

		CHECK(stringEqual(msg1, msg2));
		CHECK(!stringEqual(msg1, msg3));
		CHECK(!stringEqual(msg2, msg3));

		delete nm1;
		delete nm2;
		delete nm3;
		delete r1;
		delete r2;
	}

	SECTION("Testing basic UDP NetMessenger", "[NM][UDP]"){
		NetMessenger *nm1 = new NetMessenger(udp, 8080);
		NetMessenger *nm2 = new NetMessenger(udp, 8081);
		NetMessenger *nm3 = new NetMessenger(udp, 8083);
		Recipient r1;
		Recipient r2;

		r1->address = string("localhost");
		r2->address = string("localhost");
		r1->port = 8080;
		r2->port = 8081;

		string msg1("Hello"), msg2, msg3;
		CHECK_NOTHROW( nm1->SendTo(msg1, r1) );
		CHECK_NOTHROW( nm2->Receive() );
		CHECK_NOTHROW( nm3->Receive() );

		CHECK_NOTHROW( msg2 = nm2->GetFirstMessage() );
		CHECK_NOTHROW( msg3 = nm3->GetFirstMessage() );

		CHECK(stringEqual(msg1, msg2));
		CHECK(!stringEqual(msg1, msg3));
		CHECK(!stringEqual(msg2, msg3));

		msg2 = string("Goodbye");
		CHECK_NOTHROW(nm2->ReplyTo(msg2, r1));
		CHECK_NOTHROW(nm1->Receive());
		CHECK_NOTHROW(nm3->Receive());

		CHECK_NOTHROW(msg1 = nm1->GetFirstMessage());
		CHECK_NOTHROW(msg3 = nm3->GetFirstMessage());

		CHECK(stringEqual(msg1, msg2));
		CHECK(!stringEqual(msg1, msg3));
		CHECK(!stringEqual(msg2, msg3));

		delete nm1;
		delete nm2;
		delete nm3;
	}

	SECTION("Testing multiple TCP agents", "[NM][TCP][MULTI]"){
		
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
		vector<NetMessenger *> agents;
		vector<Recipient> addrs;
		for(int i = 0; i < 20; i += 1){
			agents[i] = new NetMessenger(tcp, 8080 + i);
			Recipient r; 
			r->address = "localhost";
			r->port = 8080 + i;
			addrs[i] = r;
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

			NetMessenger * agent1 = agents[i%20];
			NetMessenger * agent2 = agents[(i+1)% 20];
			agent1->SendTo(msg1, addrs[i+1]);
			agent2->Receive();
			msg2 = agent2->GetFirstMessage();
			CHECK(stringEqual(msg1, msg2));
			
			agent2->SendTo(msg2, addrs[i]);
			agent1->Receive();
			msg1 = agent1->GetFirstMessage();
			CHECK(stringEqual(msg1, msg2));
			msg1.erase();
			msg2.erase();
		}

		for(int i = 0; i < agents.size(); i += 1){
			delete(agents[i]);
			agents[i] = nullptr;
			addrs[i] = nullptr;
		}
	}


	SECTION("Testing multiple UDP agents", "[NM][UDP][MULTI]"){
		
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
		vector<NetMessenger *> agents;
		vector<Recipient> addrs;
		for(int i = 0; i < 20; i += 1){
			agents[i] = new NetMessenger(udp, 8080 + i);
			Recipient r; 
			r->address = "localhost";
			r->port = 8080 + i;
			addrs[i] = r;
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

			NetMessenger * agent1 = agents[i%20];
			NetMessenger * agent2 = agents[(i+1)% 20];
			agent1->SendTo(msg1, addrs[i+1]);
			agent2->Receive();
			msg2 = agent2->GetFirstMessage();
			CHECK(stringEqual(msg1, msg2));
			
			agent2->SendTo(msg2, addrs[i]);
			agent1->Receive();
			msg1 = agent1->GetFirstMessage();
			CHECK(stringEqual(msg1, msg2));
			msg1.erase();
			msg2.erase();
		}

		for(int i = 0; i < agents.size(); i += 1){
			delete(agents[i]);
			agents[i] = nullptr;
			addrs[i] = nullptr;
		}
	}

}

typedef struct{
	string name;
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
		Server * server1 = new Server(udp, 2021);
		Server * server2 = new Server(tcp, 2022);

		Recipient udp_rec;
		udp_rec->address = "localhost";
		udp_rec->port = 2021;

		Recipient tcp_rec;
		tcp_rec->address = "localhost";
		tcp_rec->port = 2022;

		NetMessenger * udp_com = new NetMessenger(udp);
		NetMessenger * tcp_com = new NetMessenger(tcp);

		udp_com->SendTo("0", udp_rec);
		udp_com->Receive();

		CHECK(stringEqual(udp_com->GetFirstMessage(), "OK"));

		tcp_com->SendTo("0", tcp_rec);
		tcp_com->Receive();

		CHECK(stringEqual(tcp_com->GetFirstMessage(), "OK"));

		delete server1;
		delete server2;
		delete udp_com;
		delete tcp_com;
	}

	SECTION("Testing addition and deletion requests in UDP", "[UDP][ADD][DEL]"){
		Server * server = new Server(udp, 2021);
		Recipient r_serv; 
		r_serv->address = "localhost";
		r_serv->port = 2021;
		NetMessenger * com = new NetMessenger(udp);
		DBConnector * db = new DBConnector("jdbc:mariadb://localhost:3306/foodservertest");
		
		com->SendTo("0", r_serv);
		ingredient_request ing1 = { "a", 2.0, "g" };
		recipe_request rec1 = { "r", "a" };

		com->Send("10|" + reqToString(ing1));
		com->Receive();
		string response = com->GetFirstMessage();
		CHECK(stringEqual("OK", response));

		string result;
		CHECK_NOTHROW(result = db->GetIngredientByIndex(0));
		CHECK(stringEqual(result, ing1.name));

		com->Send("20" + reqToString(ing1));
		com->Receive();
		response = com->GetFirstMessage();
		CHECK(stringEqual(response, "OK"));
		CHECK_NOTHROW(result = db->GetIngredientByIndex(0));
		CHECK(result.empty());

		com->Send("11|" + reqToString(rec1));
		com->Receive();
		response = com->GetFirstMessage();
		CHECK(stringEqual(response, "OK"));

		delete server;
		delete com;
		delete db;

	}

	SECTION("Testing addition and deletion requests in TCP", "[TCP][ADD][DEL]"){
		Server * server = new Server(tcp, 2021);
		Recipient r_serv; 
		r_serv->address = "localhost";
		r_serv->port = 2021;
		NetMessenger * com = new NetMessenger(tcp);
		DBConnector * db = new DBConnector("jdbc:mariadb://localhost:3306/foodservertest");

		com->SendTo("0", r_serv);
		ingredient_request ing1 = { "a", 2.0, "g" };
		recipe_request rec1 = { "r", "a" };

		com->Send("10|" + reqToString(ing1));
		com->Receive();
		string response = com->GetFirstMessage();
		CHECK(stringEqual("OK", response));

		string result;
		CHECK_NOTHROW(result = db->GetIngredientByIndex(0));
		CHECK(stringEqual(result, ing1.name));

		com->Send("20" + reqToString(ing1));
		com->Receive();
		response = com->GetFirstMessage();
		CHECK(stringEqual(response, "OK"));
		CHECK_NOTHROW(result = db->GetIngredientByIndex(0));
		CHECK(result.empty());

		com->Send("11|" + reqToString(rec1));
		com->Receive();
		response = com->GetFirstMessage();
		CHECK(stringEqual(response, "OK"));
		
		delete server;
		delete com;
		delete db;	
	}

	SECTION("Testing  reserving, and releasing in UDP", "[UDP][LIN][RES]"){
		Recipient r_serv; 
		r_serv->address = "localhost";
		r_serv->port = 2021;
		NetMessenger * com = new NetMessenger(udp);
		DBConnector * db = new DBConnector("jdbc:mariadb://localhost:3306/foodservertest");
		Server * server = new Server(udp, 2021);

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

		delete server;
		delete db;
		delete com;
	}

	SECTION("Testing matchings in UDP", "[UDP][MTC]"){
		Server * server = new Server(udp, 2021);
		Recipient r_serv;
		r_serv->address = "localhost";
		r_serv->port = 2021;
		NetMessenger * com = new NetMessenger(udp);

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

		delete server;
		delete com;
	}

}

 
