
#include <exception>
#include <mariadb/conncpp.hpp>

#include <mariadb/conncpp/PreparedStatement.hpp>
#include <mariadb/conncpp/ResultSet.hpp>
#include <string>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <memory>
#include <utility>
#include <algorithm>

#include "dbConnector.hpp"
#include "Logger.hpp"

using std::string;
using std::ifstream;
using std::unique_ptr;
using std::vector;
using std::pair;

using sql::PreparedStatement;

string DBConnector::GetIngredientByIndex(int index){
	Logger::GetInstance().log("[DBConnector::GetIngredientByIndex] index: " + std::to_string(index), debug_level::DEBUG);
	if(index < 0){
		throw std::runtime_error("[DBConnector::GetIngredientByIndex]: index cannot be negative.");
	}
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement("Select name from groceries where id - (select MIN(id) from groceries) = ?"));
	stmt->setInt(1, index);
	
	unique_ptr<sql::ResultSet> res(std::move(stmt->executeQuery()));
	if(!res->next()){
		throw std::runtime_error("[DBConnector::GetIngredientByIndex]: no ingredient at index " + std::to_string(index) + " or index out of range");
	}
	string out;
	try{
		out = static_cast<string>(res->getString(1));
	}
	catch(std::exception e){
		throw std::runtime_error("[DBConnector::GetIngredientByIndex]: failed to get name from result set.");
	}
	if(out.empty() || out.find_first_not_of(" ") == std::string::npos){
		throw std::runtime_error("[DBConnector::GetIngredientByIndex]: query returned empty string.");
	}
	return out;
}

void DBConnector::CreateIngredient(std::string name, float amount, bool mass){
	Logger::GetInstance().log("[DBConnector::CreateIngredient] name: " + name + " amount: " + std::to_string(amount), debug_level::INFO);
	if(name.empty() || name.find_first_not_of(" ") == std::string::npos){
		throw std::runtime_error("[DBConnector::CreateIngredient]: name cannot be empty.");
	}
	if(amount < 0){
		throw std::runtime_error("[DBConnector::CreateIngredient]: amount cannot be less than 0. (given: " + std::to_string(amount) + " )");
	}

	unique_ptr<PreparedStatement> stmt(connection->prepareStatement("Insert into groceries (name, amount, mass, in_use, units) values (?, ?, ?, 0, ?)"));
	stmt->setString(1, name);
	stmt->setFloat(2, amount);
	stmt->setBoolean(3, mass);
	if(mass){
		stmt->setString(4, "g");
	}
	else{
		stmt->setString(4, "ml");
	}

	stmt->executeQuery();
	connection->commit();
}

void DBConnector::CreateRecipe(string name, string instruction, string url){
	Logger::GetInstance().log("[DBConnector::CreateRecipe] name: " + name, debug_level::INFO);
	if(name.empty() || name.find_first_not_of(" ") == std::string::npos){
		throw std::runtime_error("[DBConnector::CreateRecipe]: name cannot be empty.");
	}
	if(instruction.empty() || instruction.find_first_not_of(" ") == std::string::npos){
		throw std::runtime_error("[DBConnector::CreateRecipe]: instructions cannot be empty.");
	}
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement("insert into recipes (name, instructions, url) values (?, ?, ?)"));
	stmt->setString(1,name);
	stmt->setString(2, instruction);
	stmt->setString(3, url);

	stmt->executeQuery();
	connection->commit();
}

void DBConnector::MapRecipeToIngredient(string recipe, float amount, string ingredient){
	if(recipe.empty() || recipe.find_first_not_of(" ") == std::string::npos){
		throw std::runtime_error("[DBConnector::MapRecipeToIngredient]: recipe cannot be empty");
	}
	if(ingredient.empty() || ingredient.find_first_not_of(" ") == std::string::npos){
		throw std::runtime_error("[DBConnector::MapRecipeToIngredient]: ingredient cannot be empty");
	}
	if(amount < 0){
		throw std::runtime_error("[DBConnector::MapRecipeToIngredient]: amount cannot be less than 0. (given: " + std::to_string(amount) +")");
	}
	unique_ptr<PreparedStatement> final_stmt(connection->prepareStatement(
				"insert into recipe_ingredients (ingredient_index, quantity, recipe_index) values (?, ?, ?)"));
	unique_ptr<PreparedStatement> g_id_stmt(connection->prepareStatement("select id from groceries where name = ?"));

	unique_ptr<PreparedStatement> r_id_stmt(connection->prepareStatement("select id from recipes where name = ?"));
	g_id_stmt->setString(1, ingredient);
	final_stmt->setFloat(2, amount);
	r_id_stmt->setString(1, recipe);
	
	unique_ptr<sql::ResultSet> g_res(std::move(g_id_stmt->executeQuery()));
	unique_ptr<sql::ResultSet> r_res(std::move(r_id_stmt->executeQuery()));

	if(!g_res->next()){
		Logger::GetInstance().log("[DBConnector::MapRecipeToIngredient]: grocery (" + ingredient + ") not tracked in db.", debug_level::ERROR);
		throw std::runtime_error("[DBConnector::MapRecipeToIngredient]: grocery (" + ingredient + ") not tracked in db.");
	}
	if(!r_res->next()){
		Logger::GetInstance().log("[DBConnector::MapRecipeToIngredient]: recipe (" + recipe + ") not tracked in db.", debug_level::ERROR);
		throw std::runtime_error("[DBConnector::MapRecipeToIngredient]: recipe (" + recipe + ") not tracked in db.");
	}

	int g_id = g_res->getInt(1);
	int r_id = r_res->getInt(1);

	final_stmt->setInt(1,g_id);
	final_stmt->setInt(3, r_id);
	final_stmt->execute();
	connection->commit();
}


void DBConnector::UpdateIngredient(std::string name, float amount){
	Logger::GetInstance().log("[DBConnector::UpdateIngredient] name: " + name + " amount: " + std::to_string(amount), debug_level::INFO);
	if(name.empty() || name.find_first_not_of(" ") == std::string::npos){
		throw std::runtime_error("[DBConnector::UpdateIngredient]: name cannot be empty");
	}
	string statement;
	if(amount > 0){
		statement = "update groceries set amount = amount + ? where name = ?";
	} else if (amount < 0){
		statement = "update groceries set amount = amount - ? where name = ?";
	}
	else{
		return;
	}
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement(statement));
	stmt->setFloat(1, amount * ((amount > 0) - (amount < 0)));
	stmt->setString(2,name);
	stmt->executeQuery();
}


void DBConnector::DeleteIngredient(std::string name){
	Logger::GetInstance().log("[DBConnector::DeleteIngredient] name: " + name, debug_level::INFO);
	if(name.empty() || name.find_first_not_of(" ") == std::string::npos){
		throw std::runtime_error("[DBConnector::DeleteIngredient]: name cannot be empty.");
	}

	unique_ptr<PreparedStatement> map_stmt(connection->prepareStatement("delete from recipe_ingredients where ingredient_index = (select id from groceries where name = ?)"));
	map_stmt->setString(1, name);
	map_stmt->execute();

	unique_ptr<PreparedStatement> groc_stmt(connection->prepareStatement("delete from groceries where name = ?"));
	groc_stmt->setString(1,name);
	groc_stmt->execute();
	connection->commit();
}

void DBConnector::DeleteRecipe(std::string name){
	Logger::GetInstance().log("[DBConnector::DeleteRecipe] name: " + name, debug_level::INFO);
	if(name.empty() || name.find_first_not_of(" ") == std::string::npos){
		throw std::runtime_error("[DBConnector::DeleteIngredient]: name cannot be empty.");
	}
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement("delete from recipe_ingredients where recipe_index = (select id from recipes where name = ?)"));
	stmt->setString(1, name);
	stmt->execute();
	unique_ptr<PreparedStatement> stmt2 (  connection->prepareStatement("delete from recipes where name = ?") ) ;
	stmt2->setString(1, name);
	stmt2->execute();
}

vector<pair<int,float>> DBConnector::Reserve(vector<string> ingredients, vector<float> amounts){
	Logger::GetInstance().log("[DBConnector::Reserve] ingredients", debug_level::INFO);
	if(ingredients.empty()){
		Logger::GetInstance().log("[DBConnector::Reserve] empty ingredient count", debug_level::ERROR);
		throw std::runtime_error("[DBConnector::Reserve]: ingredients cannot be empty");
	}
	if(amounts.empty()){
		Logger::GetInstance().log("[DBConnector::Reserve] empty amount count.", debug_level::ERROR);
		throw std::runtime_error("[DBConnector::Reserve]: amounts cannot be empty");
	}
	if(ingredients.size() != amounts.size()){
		Logger::GetInstance().log("[DBConnector::Reserve] ingredient:amount count mismatch", debug_level::ERROR);
		throw std::runtime_error("[DBConnector::Reserve] parameter size mismatch ( " + std::to_string(amounts.size()) + " != " + std::to_string(ingredients.size()) +" )");
	}
	string statement("select name, (amount - in_use) available from groceries where name in (");
	for(int i = 0; i < ingredients.size(); i += 1){
		if(amounts[i] < 0){
			throw std::runtime_error("[DBConnector::Reserve] amount cannot be less than 0. (given for item index " + std::to_string(i) + ": " + std::to_string(amounts[i]) + ")");
		}
		statement += " ? ";
	}
	statement += ")";
	unique_ptr<PreparedStatement> avail(connection->prepareStatement(statement));
	for(int i = 0; i < ingredients.size(); i+= 1){
		avail->setString(i+1, ingredients[i]);
	}
	unique_ptr<sql::ResultSet> res(std::move(avail->executeQuery()));
	vector<pair<int, float>> overages;
	while(res->next()){
		string ingredient = static_cast<string>(res->getString(1));
		float amount = res->getFloat(2);
		int idx = std::find(
				ingredients.begin(), 
				ingredients.end(),
			   	ingredient) - 
			ingredients.begin();
		if(amount - amounts[idx] < 0){
			overages.push_back(pair<int, float>(idx, amounts[idx] - amount));
			amounts[idx] = amount;
		}
	}
	res->close();


	statement.clear();
	statement += ("update groceries set in_use = (case ");
	for(int i = 0; i < ingredients.size(); i += 1){
		statement += "when name = ? then ?\n";
	}
	statement += "end)";
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement(statement));
	int param = 1;
	for(int i = 0; i < ingredients.size(); i += 1){
		stmt->setString(param, ingredients[i]);
		stmt->setFloat(param + 1, amounts[i]);
		param += 2;
	}
	stmt->executeUpdate();
	return overages;
}

DBConnector::DBConnector(){
	Logger::GetInstance().log("[DBConnector::DBConnector] default constructed", debug_level::INFO);
	ifstream pfile;
	string res;
	
	pfile.open("p.txt");
	if(!pfile.is_open()){
		throw std::runtime_error("[DBConnector::DBConnector()] File p.txt not found");
	}
	if(!getline(pfile, res)){
		throw std::runtime_error("[DBConnector::DBConnector()] Failed to read from p.txt");
	}
	pfile.close();

	driver = sql::mariadb::get_driver_instance();
	sql::SQLString url = ("jdbc:mariadb://localhost:3306/FoodServer?charset=utf8mb4");
	sql::Properties properties({
			{"user", "foodserver"},
			{"password", res}});
	connection = std::unique_ptr<sql::Connection>(
			driver->connect(
				url, 
				properties));
}

DBConnector::DBConnector(string address){
	Logger::GetInstance().log("[DBConnector::DBConnector] address: " + address, debug_level::INFO);
	ifstream pfile;
	string res;
	
	pfile.open("p.txt");
	if(!pfile.is_open()){
		throw std::runtime_error("[DBConnector::DBConnector(" + address + ")] File p.txt not found");
	}
	if(!getline(pfile, res)){
		throw std::runtime_error("[DBConnector::DBConnector(" + address + ")] Failed to read from p.txt");
	}
	pfile.close();

	driver = sql::mariadb::get_driver_instance();
	std::string url_str = address;
	if(url_str.find("charset=") == std::string::npos){
		url_str += "?charset=utf8mb4";
	}
	sql::SQLString url = url_str;
	sql::Properties properties({
			{"user", "foodserver"},
			{"password", res}});
	connection = std::unique_ptr<sql::Connection>(
			driver->connect(
				url, 
				properties));
	if(!connection){
		throw std::runtime_error("[DBConnector(" + address + ")] Failed to connect.");
	}
}


vector<string> DBConnector::GetIngredients(std::string recipe){
	Logger::GetInstance().log("[DBConnector::GetIngredients] for: " + recipe, debug_level::DEBUG);
	if(recipe.empty() || recipe.find_first_not_of(" ") == std::string::npos){
		throw std::runtime_error("[DBConnector::GetIngredients]: recipe cannot be empty");
	}
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement("select name from groceries join recipe_ingredients on id = ingredient_index where recipe_index = (select id from recipes where name = ?)"));
	stmt->setString(1, recipe);
	unique_ptr<sql::ResultSet> results(std::move(stmt->executeQuery()));

	vector<string> output;
	while(results->next()){
		output.push_back(static_cast<string>(results->getString(1)));
	}
	results->close();
	return output;

}

vector<string> DBConnector::GetRecipes(vector<string> ingredients){
	Logger::GetInstance().log("[DBConnector::GetRecipes] for ingredients count: " + std::to_string(ingredients.size()), debug_level::DEBUG);
	if(ingredients.empty()){
		throw std::runtime_error("[DBConnector::GetRecipes]: ingredients cannot be empty");
	}
	vector<string> output;
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement("select name from recipes join recipe_ingredients on id=recipe_index where ingredient_index = (select id from groceries where name = ?)"));
	unique_ptr<sql::ResultSet> results;
	for(string ingredient : ingredients){
		stmt->setString(1, ingredient);
		results = unique_ptr<sql::ResultSet>(std::move(std::move(stmt->executeQuery())));
		while(results->next()){
			output.push_back(static_cast<string>(results->getString(1)));
		}
	}
	results->close();
	return output;
}


vector<string> DBConnector::GetInstructions(vector<string> recipes){
	Logger::GetInstance().log("[DBConnector::GetInstructions] for recipes count: " + std::to_string(recipes.size()), debug_level::DEBUG);
	if(recipes.empty()){
		throw std::runtime_error("[DBConnector::GetInstructions] recipes cannot be empty");
	}
	for(int i = 0; i < recipes.size(); i += 1){
	}
	string query = "select instructions from recipes where name in ( ";
	for(int i = 0; i < recipes.size() - 1; i += 1){
		query.append("?, ");
	}
	query.append("? )");
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement(query));
	for(int i = 0; i < recipes.size(); i += 1){
		stmt->setString(i+1, recipes[i]);
	}
	unique_ptr<sql::ResultSet> results(std::move(stmt->executeQuery()));
	vector<string> output;
	while(results->next()){
		output.push_back(static_cast<string>(results->getString(1)));
	}
	results->close();
	return output;
}

void DBConnector::Release(vector<string> items, vector<float> amounts){
	Logger::GetInstance().log("[DBConnector::Release] items count: " + std::to_string(items.size()), debug_level::INFO);
	if(items.empty()){
		throw std::runtime_error("[DBConnector::Release]: items cannot be empty");
	}
	if(amounts.empty()){
		throw std::runtime_error("[DBConnector::Release]: amounts cannot be empty");
	}
	string releases("update groceries set in_use = case name \n");
	for(int i = 0; i < items.size(); i += 1){
		if(amounts[i] < 0){
			throw std::runtime_error("[DBConnector::Release]: amount cannot be less than 0. (given for item " + std::to_string(i) + ": " + std::to_string(amounts[i]) + ")");
		}
		releases += "when ? then in_use - ?,";
		releases += "\n";
	}
	releases += "else in_use end";
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement(releases));
	for(int i = 0; i < items.size(); i += 2){
		stmt->setString(i, items[(i-1)/2]);
		stmt->setFloat(i + 1, amounts[(i-1)/2]);
	}
	stmt->executeUpdate();
	connection->commit();
}

