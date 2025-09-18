
#include <mariadb/conncpp.hpp>

#include <string>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <memory>
#include <utility>
#include <algorithm>

#include "dbConnector.hpp"

using std::string;
using std::ifstream;
using std::unique_ptr;
using std::vector;
using std::pair;

using sql::PreparedStatement;

#include <iostream>
string DBConnector::GetIngredientByIndex(int index){
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement("Select name from groceries where id - (select min(id) from groceries) = ?"));
	stmt->setInt(1, index);
	
	sql::ResultSet *res = stmt->executeQuery();
	int i = -1;
	while(res->next() && i < index){
		i += 1;
	}
	if(index == i){
		return static_cast<string>(res->getString(0));
	}
	return string("");
}

void DBConnector::CreateIngredient(std::string name, float amount, bool mass){
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
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement("insert into recipes (name, instructions, url) values (?, ?, ?)"));
	stmt->setString(1,name);
	stmt->setString(2, instruction);
	stmt->setString(3, url);

	stmt->executeQuery();
}

void DBConnector::MapRecipeToIngredient(string recipe, float amount, string ingredient){
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement(
				"insert into recipe_ingredients (ingredient_index, quantity, recipe_index) values (( select id from groceries where name = ?), ?, (select id from recipes where name = ?))"));
	stmt->setString(1, ingredient);
	stmt->setFloat(2, amount);
	stmt->setString(3, recipe);
	stmt->execute();
}


void DBConnector::UpdateIngredient(std::string name, float amount){
	string statement;
	if(amount > 0){
		statement = "update groceries set amount + ? where name = ?";
	} else {
		statement = "update groceries set amount - ? where name = ?";
	}
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement(statement));
	stmt->setFloat(1, amount * ((amount > 0) - (amount < 0)));
	stmt->setString(2,name);
	stmt->executeQuery();
}


void DBConnector::DeleteIngredient(std::string name){
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement("delete from groceries where name = ?"));
	stmt->setString(1,name);
	stmt->execute();
	connection->commit();
}

void DBConnector::DeleteRecipe(std::string name){
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement("delete from recipe_ingredients where recipe_index = (select id from recipes where name = ?)"));
	stmt->setString(1, name);
	stmt->execute();
	unique_ptr<PreparedStatement> stmt2 (  connection->prepareStatement("delete from recipes where name = ?") ) ;
	stmt2->setString(1, name);
	stmt2->execute();
}

vector<pair<int,float>> DBConnector::Reserve(vector<string> ingredients, vector<float> amounts){
	string statement("select name, (amount - in_use) available from groceries where name in (");
	for(int i = 0; i < ingredients.size(); i += 1){
		statement += " ? ";
	}
	statement += ")";
	unique_ptr<PreparedStatement> avail(connection->prepareStatement(statement));
	for(int i = 0; i < ingredients.size(); i+= 1){
		avail->setString(i+1, ingredients[i]);
	}
	sql::ResultSet *res = avail->executeQuery();
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
	for(int i = 0; i < ingredients.size(); i += 1){
		stmt->setString(i+1, ingredients[i]);
		stmt->setFloat(i+2, amounts[i]);
	}
	stmt->executeUpdate();
	return overages;
}

DBConnector::DBConnector(){
	ifstream pfile;
	string res;
	
	pfile.open("p.txt");
	if(!pfile.is_open()){
		throw std::runtime_error("File p.txt not found");
	}
	if(!getline(pfile, res)){
		throw std::runtime_error("Failed to read from p.txt");
	}
	pfile.close();

	driver = sql::mariadb::get_driver_instance();
	sql::SQLString url = ("jdbc:mariadb://localhost:3306/foodserver");
	sql::Properties properties({
			{"user", "FoodServer"},
			{"password", res}});
	connection = std::unique_ptr<sql::Connection>(
			driver->connect(
				url, 
				properties));
}

DBConnector::DBConnector(string address){

	ifstream pfile;
	string res;
	
	pfile.open("p.txt");
	if(!pfile.is_open()){
		throw std::runtime_error("File p.txt not found");
	}
	if(!getline(pfile, res)){
		throw std::runtime_error("Failed to read from p.txt");
	}
	pfile.close();

	driver = sql::mariadb::get_driver_instance();
	sql::SQLString url = address;
	sql::Properties properties({
			{"user", "FoodServer"},
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
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement("select name, ((amount * quantity) - in_use) amount from groceries join recipe_ingredients on id = ingredient_index where recipe_index = (select id from recipes where name = ?)"));
	stmt->setString(1, recipe);
	sql::ResultSet *results = stmt->executeQuery();

	vector<string> output;
	while(results->next()){
		output.push_back(static_cast<string>(results->getString(1)));
		output.push_back(static_cast<string>(results->getString(2)));
	}
	results->close();
	return output;

}

vector<string> DBConnector::GetRecipes(vector<string> ingredients){
	vector<string> output;
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement("select name from recipes join recipe_ingredients on id=recipe_index where ingredient_index = (select id from groceries where name = ?)"));
	sql::ResultSet *results;
	for(string ingredient : ingredients){
		stmt->setString(1, ingredient);
		results = stmt->executeQuery();
		while(results->next()){
			output.push_back(static_cast<string>(results->getString(1)));
		}
	}
	results->close();
	return output;
}


vector<string> DBConnector::GetInstructions(vector<string> recipes){
	if(recipes.size() < 1){
		throw std::runtime_error("GetIntstructions with no recipes");
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
	sql::ResultSet *results = stmt->executeQuery();
	vector<string> output;
	while(results->next()){
		output.push_back(static_cast<string>(results->getString(1)));
	}
	results->close();
	return output;
}

void DBConnector::Release(vector<string> items, vector<float> amounts){
	if(items.empty() || amounts.empty()){
		std::string error = std::string("Call to db.release with items parametor of ||" + std::to_string(items.size()) + "|| and amounts parameter of ||"+ std::to_string(amounts.size()) + "||");
		throw std::runtime_error(error.c_str());
	}
	string releases("update groceries set in_use = (case \n");
	for(int i = 0; i < items.size(); i += 1){
		releases += "when name = ";
		releases += items[i];
		releases += " then ";
		releases += std::to_string(amounts[i]);
		releases += "\n";
	}
	releases += ")";
	std::cout << "\t\t" << releases << "\n";
	unique_ptr<PreparedStatement> stmt(connection->prepareStatement(releases));
	for(int i = 1; i < items.size() * 2; i += 2){
		stmt->setString(i, items[(i/2)-1]);
		stmt->setFloat(i + 1, amounts[(i/2)-1]);
	}
	stmt->executeUpdate();
}

