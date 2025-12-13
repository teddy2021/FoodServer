

#include <memory>
#include <thread>
#include <algorithm>
#include <utility>
#include <vector>
#include <iostream>
#include <string>
#include <mariadb/conncpp.hpp>

using std::string;
using std::pair;
using std::vector;
using std::cerr;


#include "Server.hpp"
#include "Enums.hpp"
#include "httpreq.hpp"
#include "IDB.hpp"
#include "dbConnector.hpp"
#include "NetMessenger.hpp"

Server::~Server(){
	if(!connections.empty()){
		for(Recipient r : connections){
			try{  
				messenger.SendTo("Shutdown", r);
			}
			catch(std::runtime_error &rt_err){
			}
		}		
		connections.clear();
	}
	groceries.clear();
}

vector<string> Server::Tokenize(string message){
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
	int category;
	try{
		category = std::stoi(type);
		std::cout << "\tCategory: " << std::to_string(category) << "\n";
	}
	catch(std::invalid_argument e){
		cerr << "[Server::ParseRequest("<<type<<")] error occured during type parsing.\n\t" <<
			e.what() << "\n";
		throw;
	}

	req rr;
	Request r = std::make_shared<req>(rr);
	switch(category){
		case 0:
			std::cout << "\tsyn\n"; 
			r->type = syn;
			break;
		case 1:
			std::cout << "\treserve\n"; 
			r->type = reserve;
			break;
		case 2:
			std::cout << "\trelease\n"; 
			r->type = release;
			break;
		case 3:
			std::cout << "\tfinalize\n"; 
			r->type = finalize;
			break;
		case 4:
			std::cout << "\tscrape\n"; 
			r->type = scrape;
			break;
		case 5:
			std::cout << "\trecipe\n"; 
			r->type = recipe;
			break;
		case 6:
			std::cout << "\tmatch\n";
			r->type = match;
			break;
		case 7:
			std::cout << "\tstop\n"; 
			r->type = stop;
			break;
		case 10:
			std::cout << "\tadd ingredient\n";
			r->type = aingredient;
			break;
		case 11:
			std::cout << "\tadd recipe\n";
			r->type = arecipe;
			break;
		case 20:
			std::cout << "\tremove ingredient\n";
			r->type = ringredient;
			break;
		case 21:
			std::cout << "\tremove recipe\n";
			r->type = rrecipe;
			break;
		default:
			std::cout << "\tFAIL\n"; 
			throw invalid_state_exception(string("[Server::ParseRequest(" +
					   	type + ")]: unknown request type."));
			break;

	}
	return r;
}

Request Server::ParseRequest(string message){
	vector<string> tokens = Tokenize(message);
	Request output = ParseRequestType(tokens[0]);
	vector<string> params(tokens.begin() + 1, tokens.end());
	output->parameters = params;
	return output;
}

void Server::CheckRequest(string fcn, Request request){

	if(NULL == request){
		throw null_request_error(string("[Server::" + fcn + "]: null request."));
	}
	requests type = request->type;
	if( (type != syn && type != finalize && type != stop) && request->parameters.empty()){
		std::cout << request_text[request->type]<< "\n";
		Respond(request, "300");
		throw empty_parameter_exception(string("[Server::" + fcn + "(" +
					string(request_text[request->type]) + 
					", <||" + std::to_string(request->parameters.size()) + "||>, " +
					std::to_string(request->connection) + ")] empty parameter."));
	}
}


Server::Server(protocol_type type): messenger(type), db(), connections(){
	const auto processor_count = std::thread::hardware_concurrency();
	worker_count = sysconf(_SC_NPROCESSORS_ONLN);
	next_worker = 1;
}

Server::Server(protocol_type type, unsigned short int port): messenger(type, port), db(), connections(){
	worker_count = std::thread::hardware_concurrency();
	next_worker = 1;
}

Server::Server(NetMessenger net): messenger(net), db(), connections(){
	db = std::make_unique<DBConnector>();
	const auto processor_count = std::thread::hardware_concurrency();
	worker_count = sysconf(_SC_NPROCESSORS_ONLN);
	next_worker = 1;
}


void Server::Listen(){
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
		std::cerr << "Failed to dispatch request " << request_text[request->type] << " with parameters <";
		for(int i = 0; i < request->parameters.size(); i += 1){
			std::cerr << " " << request->parameters[i];
		}
		std::cerr << ">; an SQL exception ocurred.\n\t" << sd_except.what() << "\n";
		Respond(request, "100");
	}
	catch(const std::length_error &len_err){
		std::cerr << "Failed to dispatch request " << request_text[request->type] << " with parameters <";
		for(int i = 0; i < request->parameters.size(); i += 1){
			std::cerr << " " << request->parameters[i];
		}
		std::cerr << ">; a length error ocurred.\n\t" << len_err.what() << "\n";
		Respond(request, "600");
	}
	catch(invalid_state_exception &is_exception){
		std::cerr << "Failed to dispatch request " << request_text[request->type] << " with parameters <";
		for(int i = 0; i < request->parameters.size(); i += 1){
			std::cerr << " " << request->parameters[i];
		}
		std::cerr << ">; an invalid state exception ocurred.\n\t" << is_exception.what() << "\n";
		Respond(request, "500");
	}
	catch(empty_response_parameter_exception &erp_exception){
		std::cerr << "Failed to dispatch request " << request_text[request->type] << " with parameters <";
		for(int i = 0; i < request->parameters.size(); i += 1){
			std::cerr << " " << request->parameters[i];
		}
		std::cerr << ">; an empty response parameter ocurred.\n\t" << erp_exception.what() << "\n";
		Respond(request, "200");
	}
	catch(std::runtime_error &rt_err){
		std::cerr << "Failed to dispatch request " << request_text[request->type] << " with parameters <";
		for(int i = 0; i < request->parameters.size(); i += 1){
			std::cerr << " " << request->parameters[i];
		}
		std::cerr << ">; a runtime error ocured.\n\t" << rt_err.what() << "\n";
	}
	catch(...){
		std::exception_ptr exception = std::current_exception();
		std::cerr << "Failed to dispatch request " << request_text[request->type] << " with parameters <";
		for(int i = 0; i < request->parameters.size(); i += 1){
			std::cerr << " " << request->parameters[i];
		}
		std::cerr << ">; an exception/error ocurred.\n";
		std::rethrow_exception(exception);
		Respond(request, "1000");
	}
	return true;
}

void Server::AddToGroceryList(vector<pair<string, float>> groceriesAndMinimums){
	if(groceriesAndMinimums.empty()){
		throw empty_parameter_exception("[Server::AddToGroceryList(<|| " + 
				std::to_string(groceriesAndMinimums.size()) + 
				" ||>)] empty grocery list.");
	}
	for(int i = 0; i < groceriesAndMinimums.size(); i += 1){
		string item = groceriesAndMinimums[i].first;
		float min = groceriesAndMinimums[i].second;
		if(groceries.contains(item)){
			groceries[item] += min;
		} 
		else {
			groceries.insert({item, min});
		}
	}
}

void Server::AddIngredients(Request request){
	for(auto it = request->parameters.begin(); it != request->parameters.end(); it += 3 ){
		string ingredient = *it;
		float amount = std::stof(*(it + 1));
		string unit = *(it + 2);
		try{
			db->CreateIngredient(ingredient, amount, false);
		}
		catch(...){
			db->UpdateIngredient(ingredient, amount);
		}
	}
	Respond(request, "OK");
}

void Server::RemoveIngredients(Request request){
	CheckRequest("RemoveIngredients", request);
	for(string ingredient : request->parameters){
		db->DeleteIngredient(ingredient);
	}
	Respond(request, "OK");
}

void Server::Reserve(Request request){
	CheckRequest("Reserve", request);
	vector<string> ingredients;
	vector<float> amounts;
	for(int i = 0; i < request->parameters.size(); i += 2){
		try{
			amounts.push_back(std::stof(request->parameters[i+1]));
			ingredients.push_back(request->parameters[i]);
		}
		catch(...){

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
	CheckRequest("Release", request);
	vector<string> remainingItems;
	vector<float> remainingAmounts;
	for(int i = 0; i < request->parameters.size(); i += 2){
		string item = request->parameters[i];
		float amount = std::stof(request->parameters[i+1]);
		if(amount == 0){
			continue;
		}
		if(groceries.contains(item)	&& groceries[item] > amount){
			groceries[item] -= amount;
		}
		else if(groceries.contains(item) && amount <= groceries[item]){
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
	CheckRequest("AddRecipe", request);
	string name = request->parameters[0];
	string instructions = request->parameters[1];
	db->CreateRecipe(name, instructions);
	Respond(request, "OK");
}

void Server::RemoveRecipe(Request request){
	CheckRequest("RemoveRecipe", request);
	db->DeleteRecipe(request->parameters[0]);
	Respond(request, "OK");
}


void Server::RespondWithGroceries(Request request){
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
	CheckRequest("RespondWithIngredientsAndInstructions", request);
	if(ingredients.empty() || instructions.empty()){
		throw empty_response_parameter_exception(string(
					"[Server::RespondWithIngredientsAndInstructions(ingredients: <||" + 
					std::to_string(ingredients.size()) + 
				   "||>, instructions: <||" + std::to_string(
					   instructions.size()
					   ) + "||> )] empty parameter.")); 
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
	CheckRequest("RespondWithRecipes", request);
	if(recipes.empty()){
		throw empty_response_parameter_exception(string("[Server::RespondWithRecipes(<||" +  
					std::to_string(recipes.size()) + "||>)] empty parameter."));
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
	SendToNthConnection(message, request->connection);
}

void Server::SendToNthConnection(string message, int n){
	if(message.empty()){
		throw empty_response_exception("[Server::SendToNthConnection('', " + 
			   std::to_string(n) + ")] empty message.");
	}
	if(n < 0 || n > connections.size()){
		throw std::out_of_range(string("[Server::SendToNthConnection(" + message + 
					", " + std::to_string(n) + ")]: " + std::to_string(n) +	
					" out of bounds for range of " + 
					std::to_string(connections.size()) + "."));
	}
	if(n < connections.size() && n >= 0){
		Recipient addr;
	    addr = connections[n];
		messenger.SendTo(message, addr);
	}
}



