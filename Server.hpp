
#include "NetMessenger.hpp"
#include "IDB.hpp"
#include "Enums.hpp"

#include <memory>
#include <utility>
#include <unordered_map>


struct req {
	requests type;
	std::vector<std::string> parameters;
	int connection;
};

typedef std::shared_ptr<struct req> Request;

class Server{

	private:
		NetMessenger messenger;
		std::unique_ptr<IDB> db;
		std::unordered_map<std::string, float> groceries;

		unsigned short int next_worker;
		int worker_count;
		std::vector<Recipient> connections;

		/**
		 * Splits a request into tokens by pipe seperators
		 **/
		std::vector<std::string> Tokenize(std::string message);
		/**
	 * Extracts the type of request.
		 **/
		Request ParseRequestType(std::string type);
		/**
		 * Gets the type, and parameters of a given request
		 **/
		Request ParseRequest(std::string message);
		/**
		 * Checks that the type and parameters are correct for a supplied function.
		 * Will throw the correct error/exception if there is a mismatch or not enough arguments.
		 * note: the mapping is hard coded in the body of the function.
		 **/
		void CheckRequest(std::string fcn, Request request);

		/**
		 * UNIMPLEMENTED
		 * Used to send a message to the nth connected client (in temporal order)
		 **/
		void SendToNthConnection(std::string message, int n);
		/**
		 * Adds a set of groceries to the running grocery list.
		 **/
		void AddToGroceryList(std::vector<std::pair<std::string, float>> groceriesAndMinimums);

		// this block expects request parameters in the form of
		// (1)ingredients, followed by their respective
		// (2)amounts in [1,2,1,2] order.
		// for example if 5 peppers and 10 bannanas are on the list then the 
		// parameters would appear as [peppers, 5, bananas, 10].
		/**
		 * Used to add items to the pantry
		 **/
		void AddIngredients(Request request);
		/**
		 * Used to remove items from the pantry
		 **/
		void RemoveIngredients(Request request);
		/**
		 * Used to mark an ingredient as 'in use' to prevent using more than is
		 * available in the pantry
		 **/
		void Reserve(Request request);
		/**
		 * Used to remove the status of 'in use' from an amount of ingredient(s).
		 **/
		void Release(Request request);
		
		/**
		 * Used to add recipes to the DB
		 * Parameters should be the name of the recipe as a string, and the 
		 * instructions as a string.
		 **/
		void AddRecipe(Request request);
		/**
		 * Used to remove a recipe from the DB
		 * Parameter should be the name of the recipe as a string
		 **/
		void RemoveRecipe(Request request);


		/**
		 * Used to return the needed groceries and respective amounts to the 
		 * client
		 **/
		void RespondWithGroceries(Request request);

		/**
		 * Used to retrieve the complete recipe from the db. 
		 * parameter is the list of ingrsedients
		 **/
		void GetIngredientsAndInstructions(Request request);
		/**
		 * Used to return completed recipes to the client.
		 **/
		void RespondWithIngredientsAndInstructions(std::vector<std::string> ingredients,
				std::vector<std::string> instructions, Request request);

		/**
		 * Used to get recipes that use the given ingredients.
		 * parameter is a list of ingredient names.
		 **/
		void MatchRecipe(Request request);
		/**
		 * used to return recipes to the client.
		 **/
		void RespondWithRecipes(std::vector<std::string> recipes, Request request);
		
		/**
		 * Used to decide on an action for and handle a request internally.
		 * Parameter is the request returned from ParseRequest.
		 **/
		bool DoRequest(Request request);

		void Respond(Request request, std::string message);

	public:

		Server(){};
		Server(protocol_type type);
		Server(protocol_type type, unsigned short port);
		Server(NetMessenger net);

		Server(NetMessenger & net, std::unique_ptr<IDB> dbconn): 
			messenger(net), 
			db(std::move(dbconn)), 
			connections(){ 
			worker_count = std::thread::hardware_concurrency();
			next_worker = 1;
		};
		~Server();

		Server(Server &other) = delete;
		Server & operator=(const Server &other) = delete;
Server & operator=(Server &&other){
			if(this != &other){
				messenger = std::move(other.messenger);
				db = std::move(other.db);
				groceries = std::move(other.groceries);
				next_worker = other.next_worker;
				worker_count = other.worker_count;
				connections = std::move(other.connections);
			}
			return *this;
		}
	
		friend void swap(Server first, Server second){
			swap(first.messenger, second.messenger);
			std::swap(first.db, second.db);
			std::swap(first.groceries, second.groceries);

			std::swap(first.next_worker, second.next_worker);
			std::swap(first.worker_count, second.worker_count);
			std::swap(first.connections, second.connections);
		}
		void Listen();
};

class empty_parameter_exception: public std::exception{
	private:
		std::string message;
	public:
		empty_parameter_exception(std::string msg): message(msg){};
		char const * what() const throw(){
			return message.c_str();
		}
};

class null_request_error: public std::exception{
	private:
		std::string message;
	public:
		null_request_error(std::string msg): message(msg){};
		char const* what() const throw(){
			return message.c_str();
		}
};

class empty_response_parameter_exception: public std::exception{
	private:
		std::string message;
	public:
		empty_response_parameter_exception(std::string msg): message(msg){};
		char const * what() const throw(){
			return message.c_str();
		}
};

class empty_response_exception: public std::exception{
	private:
		std::string message;
	public:
		empty_response_exception(std::string msg): message(msg){};
		char const * what() const throw(){
			return message.c_str();
		}
};

