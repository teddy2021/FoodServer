

#include <cmath>
#include <memory>
#include <thread>
#include <algorithm>
#include <utility>
#include <vector>
#include <string>
#include <mariadb/conncpp.hpp>

using std::string;
using std::pair;
using std::vector;


#include "Server.hpp"
#include "Enums.hpp"
#include "httpreq.hpp"
#include "IDB.hpp"
#include "dbConnector.hpp"
#include "NetMessenger.hpp"
#include "Logger.hpp"

Server::~Server(){
	Logger::GetInstance().log("[Server::~Server] shutting down", debug_level::INFO);
	if(!connections.empty()){
		for(Recipient r : connections){
			try{  
				messenger.SendTo("Shutdown", r);
			}
			catch(std::runtime_error &rt_err){
				Logger::GetInstance().log("[Server::~Server] Failed to send shutdown to connection: " + std::string(rt_err.what()), debug_level::WARN);
			}
		}		
		connections.clear();
	}
	groceries.clear();
}

vector<string> Server::Tokenize(string message){
	Logger::GetInstance().log("[Server::Tokenize] tokenizing message: " + message.substr(0, 50), debug_level::DEBUG);
	vector<string> output;
	int start = 0;
	int end = 0;
	for(int i = 0; i < message.size(); i += 1 ){
		if(message.at(i) == '|'){
			output.push_back(message.substr(start, end));
			start = i + 1;
			end = 0;
		}
		else{
			end += 1;
		}
	}
	output.push_back(message.substr(start, message.size() ));
	return output;
}

Request Server::ParseRequestType(string type){
	Logger::GetInstance().log("[Server::ParseRequestType] parsing request type: " + type, debug_level::DEBUG);
	int category;
	try{
		category = std::stoi(type);
		Logger::GetInstance().log( "\tCategory: " + std::to_string(category) + "\n", debug_level::INFO );
	}
	catch(std::invalid_argument e){
		Logger::GetInstance().log("[Server::ParseRequestType] Failed to parse request type: " + type + " - " + e.what(), debug_level::ERROR);
		throw;
	}

	req rr;
	Request r = std::make_shared<req>(rr);
	switch(category){
		case 0:
			Logger::GetInstance().log( "\tsyn\n", debug_level::DEBUG ); 
			r->type = syn;
			break;
		case 1:
			Logger::GetInstance().log( "\treserve\n", debug_level::DEBUG ); 
			r->type = reserve;
			break;
		case 2:
			Logger::GetInstance().log( "\trelease\n", debug_level::DEBUG ); 
			r->type = release;
			break;
		case 3:
			Logger::GetInstance().log( "\tfinalize\n", debug_level::DEBUG );
			r->type = finalize;
			break;
		case 4:
			Logger::GetInstance().log( "\tscrape\n", debug_level::DEBUG );
			r->type = scrape;
			break;
		case 5:
			Logger::GetInstance().log( "\trecipe\n", debug_level::DEBUG ); 
			r->type = recipe;
			break;
		case 6:
			Logger::GetInstance().log( "\tmatch\n", debug_level::DEBUG );
			r->type = match;
			break;
		case 7:
			Logger::GetInstance().log( "\tstop\n", debug_level::DEBUG ); 
			r->type = stop;
			break;
		case 10:
			Logger::GetInstance().log( "\tadd ingredient\n", debug_level::DEBUG );
			r->type = aingredient;
			break;
		case 11:
			Logger::GetInstance().log( "\tadd recipe\n", debug_level::DEBUG );
			r->type = arecipe;
			break;
		case 20:
			Logger::GetInstance().log( "\tremove ingredient\n", debug_level::DEBUG );
			r->type = ringredient;
			break;
		case 21:
			Logger::GetInstance().log( "\tremove recipe\n", debug_level::DEBUG );
			r->type = rrecipe;
			break;
		default:
			Logger::GetInstance().log("[Server::ParseRequestType] Unknown request type: " + type, debug_level::ERROR);
			throw invalid_state_exception(string("[Server::ParseRequest(" +
					   	type + ")]: unknown request type."));
			break;

	}
	return r;
}

Request Server::ParseRequest(string message){
	Logger::GetInstance().log("[Server::ParseRequest] parsing message: " + message.substr(0, 50), debug_level::DEBUG);
	vector<string> tokens = Tokenize(message);
	Request output = ParseRequestType(tokens[0]);
	vector<string> params(tokens.begin() + 1, tokens.end());
	output->parameters = params;
	return output;
}

void Server::CheckRequest(string fcn, Request request){
	Logger::GetInstance().log("[Server::CheckRequest] validating request for: " + fcn, debug_level::DEBUG);

	if(NULL == request){
		Logger::GetInstance().log("[Server::CheckRequest] Null request for: " + fcn, debug_level::ERROR);
		throw null_request_error(string("[Server::" + fcn + "]: null request."));
	}
	requests type = request->type;
	if( (type != syn && type != finalize && type != stop) && request->parameters.empty()){
		Logger::GetInstance().log( string(request_text[request->type]), debug_level::DEBUG );
		Respond(request, "300");
		std::string error_msg = string("[Server::" + fcn + "(" +
					string(request_text[request->type]) + 
					", <||" + std::to_string(request->parameters.size()) + "||>, " +
					std::to_string(request->connection) + ")] empty parameter.");
		Logger::GetInstance().log("[Server::CheckRequest] " + error_msg, debug_level::ERROR);
		throw empty_parameter_exception(error_msg);
	}
}


Server::Server(protocol_type type): messenger(type), db(), connections(){
	Logger::GetInstance().log("[Server::Server] protocol type", debug_level::INFO);
	const auto processor_count = std::thread::hardware_concurrency();
	worker_count = sysconf(_SC_NPROCESSORS_ONLN);
	next_worker = 1;
}

Server::Server(protocol_type type, unsigned short int port): messenger(type, port), db(), connections(){
	Logger::GetInstance().log("[Server::Server] protocol type and port: " + std::to_string(port), debug_level::INFO);
	worker_count = std::thread::hardware_concurrency();
	next_worker = 1;
}

Server::Server(NetMessenger net): messenger(net), db(), connections(){
	Logger::GetInstance().log("[Server::Server] NetMessenger", debug_level::INFO);
	db = std::make_unique<DBConnector>();
	const auto processor_count = std::thread::hardware_concurrency();
	worker_count = sysconf(_SC_NPROCESSORS_ONLN);
	next_worker = 1;
}


void Server::Listen(){
	Logger::GetInstance().log("[Server::Listen] listening for requests", debug_level::INFO);
	bool running = true;
	while(running){
		messenger.Receive();
		string message = messenger.GetFirstMessage();
		httpreq h_request;
		Request p_request;
		string first_seven = message.substr(0,7);
		if(std::find(request_methods.begin(), request_methods.end(), first_seven) != request_methods.end()){
			h_request = httpreq(message);
			p_request = ParseRequest(h_request.Custom());
		}
		else{
			p_request = ParseRequest(message);
		}
		auto prev_con = std::find(connections.begin(), connections.end(), messenger.GetRemoteEndpoint());
		if(prev_con == connections.end()){
			connections.push_back(messenger.GetRemoteEndpoint());
			p_request->connection = connections.size() - 1;
		}
		else{
			p_request->connection = (int)(prev_con - connections.begin());
		}
		running = DoRequest(p_request);
	}
}


bool Server::DoRequest(Request request){
	Logger::GetInstance().log("[Server::DoRequest] processing request: " + std::string(request_text[request->type]), debug_level::DEBUG);
	CheckRequest("DoRequest", request);
	// Need to add try catch block and behavior within for each exception type so
	// the server doesn't crash on failure
	try{
		switch(request->type){
			case syn:
				Respond(request, "OK");
				break;
			case reserve:
				Reserve(request);
				break;
			case release:
				Release(request);
				break;
			case finalize:
				Respond(request, "Finalized.");
				break;
			case scrape:
				break;
			case recipe:
				GetIngredientsAndInstructions(request);
				break;
			case match:
				MatchRecipe(request);
				break;
			case stop:
				Respond(request, "Shutting down...");
				return false;
			case arecipe:
				AddRecipe(request);
				break;
			case aingredient:
				AddIngredients(request);
				break;
			case rrecipe:
				RemoveRecipe(request);
				break;
			case ringredient:
				RemoveIngredients(request);
				break;
			default:
				break;
		}
	}
	catch(const sql::SQLDataException &sd_except){
		Logger::GetInstance().log("[Server::DoRequest] SQL exception: " + std::string(sd_except.what()), debug_level::ERROR);
		Logger::GetInstance().log( "Failed to dispatch request " + string( request_text[request->type] ) + " with parameters <", debug_level::DEBUG );
		for(int i = 0; i < request->parameters.size(); i += 1){
			Logger::GetInstance().log( " " + request->parameters[i], debug_level::DEBUG );
		}
		Logger::GetInstance().log( ">; an SQL exception ocurred.", debug_level::DEBUG );
		Respond(request, "100");
	}
	catch(const std::length_error &len_err){
		Logger::GetInstance().log("[Server::DoRequest] Length error: " + std::string(len_err.what()), debug_level::ERROR);
		Logger::GetInstance().log( "Failed to dispatch request " + string( request_text[request->type] ) + " with parameters <", debug_level::DEBUG );
		for(int i = 0; i < request->parameters.size(); i += 1){
			Logger::GetInstance().log( " " + request->parameters[i], debug_level::DEBUG );
		}
		Logger::GetInstance().log( ">; an SQL exception ocurred.", debug_level::DEBUG );
		Respond(request, "600");
	}
	catch(invalid_state_exception &is_exception){
		Logger::GetInstance().log("[Server::DoRequest] Invalid state exception: " + std::string(is_exception.what()), debug_level::ERROR);
		Logger::GetInstance().log( "Failed to dispatch request " + string( request_text[request->type] ) + " with parameters <", debug_level::DEBUG );
		for(int i = 0; i < request->parameters.size(); i += 1){
			Logger::GetInstance().log( " " + request->parameters[i], debug_level::DEBUG );
		}
		Logger::GetInstance().log( ">; an SQL exception ocurred.", debug_level::DEBUG );
		Respond(request, "500");
	}
	catch(empty_response_parameter_exception &erp_exception){
		Logger::GetInstance().log("[Server::DoRequest] Empty response parameter exception: " + std::string(erp_exception.what()), debug_level::WARN);
		Logger::GetInstance().log( "Failed to dispatch request " + string( request_text[request->type] ) + " with parameters <", debug_level::DEBUG );
		for(int i = 0; i < request->parameters.size(); i += 1){
			Logger::GetInstance().log( " " + request->parameters[i], debug_level::DEBUG );
		}
		Logger::GetInstance().log( ">; an SQL exception ocurred.", debug_level::DEBUG );
		Respond(request, "200");
	}
	catch(std::runtime_error &rt_err){
		Logger::GetInstance().log("[Server::DoRequest] Runtime error: " + std::string(rt_err.what()), debug_level::ERROR);
		Logger::GetInstance().log( "Failed to dispatch request " + string( request_text[request->type] ) + " with parameters <", debug_level::DEBUG );
		for(int i = 0; i < request->parameters.size(); i += 1){
			Logger::GetInstance().log( " " + request->parameters[i], debug_level::DEBUG );
		}
		Logger::GetInstance().log( ">; a runtime exception ocurred.", debug_level::DEBUG );
	}
	catch(...){
		Logger::GetInstance().log("[Server::DoRequest] Unknown exception", debug_level::ERROR);
		std::exception_ptr exception = std::current_exception();
		Logger::GetInstance().log( "Failed to dispatch request " + string( request_text[request->type] ) + " with parameters <", debug_level::DEBUG );
		for(int i = 0; i < request->parameters.size(); i += 1){
			Logger::GetInstance().log( " " + request->parameters[i], debug_level::DEBUG );
		}
		Logger::GetInstance().log( ">; an error/exception ocurred.", debug_level::DEBUG );
		std::rethrow_exception(exception);
		Respond(request, "1000");
	}
	return true;
}

void Server::AddToGroceryList(vector<pair<string, float>> groceriesAndMinimums){
	Logger::GetInstance().log("[Server::AddToGroceryList] adding to grocery list, count: " + std::to_string(groceriesAndMinimums.size()), debug_level::DEBUG);
	if(groceriesAndMinimums.empty()){
		std::string error_msg = "[Server::AddToGroceryList(<|| " + 
				std::to_string(groceriesAndMinimums.size()) + 
				" ||>)] empty grocery list.";
		Logger::GetInstance().log("[Server::AddToGroceryList] " + error_msg, debug_level::ERROR);
		throw empty_parameter_exception(error_msg);
	}
	for(int i = 0; i < groceriesAndMinimums.size(); i += 1){
		string item = groceriesAndMinimums[i].first;
		float min = groceriesAndMinimums[i].second;
		if(groceries.find(item) != groceries.end()){
			groceries[item] += min;
		} 
		else {
			groceries.insert({item, min});
		}
	}
}

void Server::AddIngredients(Request request){
	Logger::GetInstance().log("[Server::AddIngredients] adding ingredients, count: " + std::to_string(request->parameters.size()), debug_level::INFO);
	for(auto it = request->parameters.begin(); it != request->parameters.end(); it += 3 ){
		string ingredient = *it;
		float amount = std::stof(*(it + 1));
		string unit = *(it + 2);
		try{
			db->CreateIngredient(ingredient, amount, false);
		}
		catch(...){
			Logger::GetInstance().log("[Server::AddIngredients] ingredient " + ingredient + " already exists in db, updating instead.", debug_level::INFO);
			db->UpdateIngredient(ingredient, amount);
		}
	}
	Respond(request, "OK");
}

void Server::RemoveIngredients(Request request){
	Logger::GetInstance().log("[Server::RemoveIngredients] removing ingredients: " + std::to_string(request->parameters.size()), debug_level::INFO);
	CheckRequest("RemoveIngredients", request);
	for(string ingredient : request->parameters){
		db->DeleteIngredient(ingredient);
	}
	Respond(request, "OK");
}

void Server::Reserve(Request request){
	Logger::GetInstance().log("[Server::Reserve] reserving ingredients", debug_level::INFO);
	CheckRequest("Reserve", request);
	vector<string> ingredients;
	vector<float> amounts;
	for(int i = 0; i < request->parameters.size(); i += 2){
		try{
			amounts.push_back(std::stof(request->parameters[i+1]));
			ingredients.push_back(request->parameters[i]);
		}
		catch(...){
			Logger::GetInstance().log("[Server::Reserve] could not parse " + request->parameters[i+1] + " as a float value.", debug_level::ERROR);
		}
	}

	vector<pair<int, float>> result = db->Reserve(ingredients, amounts);
	vector<pair<string, float>> remainder;
	if(0 < result.size()){
		for(int i = 0; i < result.size(); i += 1){
			string ingredient = db->GetIngredientByIndex(result[i].first);
			remainder.push_back(pair<string, float>{ingredient, result[i].second});
		}
		AddToGroceryList(remainder);
	}
	if(!groceries.empty()){  
		RespondWithGroceries(request);
	}
	else{
		Respond(request, "OK");
	}
}

void Server::Release(Request request){
	Logger::GetInstance().log("[Server::Release] releasing ingredients", debug_level::INFO);
	CheckRequest("Release", request);
	vector<string> remainingItems;
	vector<float> remainingAmounts;
	for(int i = 0; i < request->parameters.size(); i += 2){
		string item = request->parameters[i];
		float amount = std::stof(request->parameters[i+1]);
		if(amount == 0){
			continue;
		}
		if(groceries.find(item) != groceries.end() && groceries[item] > amount){
			groceries[item] -= amount;
		}
		else if(groceries.find(item) != groceries.end() && amount <= groceries[item]){
			amount -= groceries[item];
			groceries.erase(item);
			if(amount > 0){
				remainingItems.push_back(item);
				remainingAmounts.push_back(amount);
			}
		}
		else{
			remainingItems.push_back(item);
			remainingAmounts.push_back(amount);
		}
	}
	if(!remainingAmounts.empty() && !remainingItems.empty()){
		db->Release(remainingItems, remainingAmounts);
	}
	else{  
		Respond(request, "OK");
	}
}

void Server::AddRecipe(Request request){
	Logger::GetInstance().log("[Server::AddRecipe] adding recipe: " + request->parameters[0], debug_level::INFO);
	CheckRequest("AddRecipe", request);
	string name = request->parameters[0];
	string instructions = request->parameters[1];
	db->CreateRecipe(name, instructions);
	Respond(request, "OK");
}

void Server::RemoveRecipe(Request request){
	Logger::GetInstance().log("[Server::RemoveRecipe] removing recipe: " + request->parameters[0], debug_level::INFO);
	CheckRequest("RemoveRecipe", request);
	db->DeleteRecipe(request->parameters[0]);
	Respond(request, "OK");
}


void Server::RespondWithGroceries(Request request){
	Logger::GetInstance().log("[Server::RespondWithGroceries] responding with groceries", debug_level::DEBUG);
	CheckRequest("RespondWithGroceries", request);
	string response;
	for(auto it = groceries.begin(); it != groceries.end(); it ++){
		response += it->first;
		response += " : ";
		response += std::to_string(it->second);
		response += "\n";
	}
	if(response == ""){
		return ;
	}
	Respond(request, response);
}


void Server::GetIngredientsAndInstructions(Request request){
	Logger::GetInstance().log("[Server::GetIngredientsAndInstructions] getting ingredients and instructions", debug_level::INFO);
	CheckRequest("GetIngredientsAndInstructions", request);
	vector<string> instructions = db->GetInstructions(request->parameters);
	vector<string> ingredients;
	for(int i = 0; i < request->parameters.size(); i += 1){
		 vector<string> subset = db->GetIngredients(request->parameters[i]);
		 for(int j = 0; j < subset.size(); j += 2){
			auto it = find(ingredients.begin(), ingredients.end(), subset[j]);
			 if( it != ingredients.end()){
				 try{
					 float amount = std::stof(subset[j+1]);
					 float present = std::stof(*(it + 1));
					 *(it+1) = std::to_string(amount + present);
				 }
				 catch(std::exception e){
					 Logger::GetInstance().log("[Server::GetIngredientsAndInstructions] could not parse either amount (" + subset[j+1] + ") or present amount (" + *(it+1) + ") as float values.", debug_level::ERROR);
				 }
			 }
			 else{
				 ingredients.push_back(subset[j]);
				 ingredients.push_back(subset[ j+1 ]);
			 }
		 }
	}
	if(!ingredients.empty() && !instructions.empty()){  
		RespondWithIngredientsAndInstructions(ingredients, instructions, request);
	}
	else{
		Respond(request, "404 Not Found.");
	}
}

void Server::RespondWithIngredientsAndInstructions(vector<string> ingredients,
		vector<string> instructions, Request request){
	Logger::GetInstance().log("[Server::RespondWithIngredientsAndInstructions] responding with ingredients: " + 
		std::to_string(ingredients.size()) + ", instructions: " + std::to_string(instructions.size()), debug_level::DEBUG);
	CheckRequest("RespondWithIngredientsAndInstructions", request);
	if(ingredients.empty() || instructions.empty()){
		std::string error_msg = string(
					"[Server::RespondWithIngredientsAndInstructions(ingredients: <||" + 
					std::to_string(ingredients.size()) + 
				   "||>, instructions: <||" + std::to_string(
					   instructions.size()
					   ) + "||> )] empty parameter.");
		Logger::GetInstance().log("[Server::RespondWithIngredientsAndInstructions] " + error_msg, debug_level::ERROR);
		throw empty_response_parameter_exception(error_msg);
	}
	string response;
	for(int i = 0; i < ingredients.size(); i += 1){
		response += ingredients[i];
		response += "\n";
	}
	response += "\n";
	for(int i = 0; i < instructions.size(); i += 1){
		response += instructions[i];
		response += "\n";
	}
	Respond(request, response);
}


void Server::MatchRecipe(Request request){
	Logger::GetInstance().log("[Server::MatchRecipe] matching recipes", debug_level::INFO);
	CheckRequest("MatchRecipe", request);
	vector<string> results = db->GetRecipes(request->parameters);
	string response = "";
	for(int i = 0; i < results.size(); i += 1){
		response += results[i];
		response += "\n";
	}
	if(!response.empty()){  
		Respond(request, response);
	}
	else{
		Respond(request, "No matching recipes.");
	}
}

void Server::RespondWithRecipes(vector<string> recipes, Request request){
	Logger::GetInstance().log("[Server::RespondWithRecipes] responding with recipes: " + std::to_string(recipes.size()), debug_level::DEBUG);
	CheckRequest("RespondWithRecipes", request);
	if(recipes.empty()){
		std::string error_msg = string("[Server::RespondWithRecipes(<||" +  
					std::to_string(recipes.size()) + "||>)] empty parameter.");
		Logger::GetInstance().log("[Server::RespondWithRecipes] " + error_msg, debug_level::ERROR);
		throw empty_response_parameter_exception(error_msg);
	}
	if(recipes.size() > 0){
		string response;
		for(int i = 0; i < recipes.size(); i += 1){
			response += recipes[i];
			response += "\n";
		}
		SendToNthConnection(response, request->connection);
	}
}

void Server::Respond(Request request, string message){
	Logger::GetInstance().log("[Server::Respond] sending response to connection: " + std::to_string(request->connection), debug_level::DEBUG);
	SendToNthConnection(message, request->connection);
}

void Server::SendToNthConnection(string message, int n){
	Logger::GetInstance().log("[Server::SendToNthConnection] sending to connection index: " + std::to_string(n), debug_level::DEBUG);
	if(message.empty()){
		std::string error_msg = "[Server::SendToNthConnection('', " + 
			   std::to_string(n) + ")] empty message.";
		Logger::GetInstance().log("[Server::SendToNthConnection] " + error_msg, debug_level::ERROR);
		throw empty_response_exception(error_msg);
	}
	if(n < 0 || n > connections.size()){
		std::string error_msg = string("[Server::SendToNthConnection(" + message + 
					", " + std::to_string(n) + ")]: " + std::to_string(n) +	
					" out of bounds for range of " + 
					std::to_string(connections.size()) + ".");
		Logger::GetInstance().log("[Server::SendToNthConnection] " + error_msg, debug_level::ERROR);
		throw std::out_of_range(error_msg);
	}
	if(n < connections.size() && n >= 0){
		Recipient addr;
	    addr = connections[n];
		messenger.SendTo(message, addr);
	}
}



