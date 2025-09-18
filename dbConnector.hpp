
#pragma once
#include <mariadb/conncpp.hpp>
#include "Enums.hpp"
#include <utility>

class DBConnector {

	private:
		sql::Driver * driver;
		std::unique_ptr<sql::Connection> connection;
		
	public:
		// Constructors/Destructors
		DBConnector();
		DBConnector(std::string address);

		// Recipe/Ingredient manipulations
		void CreateIngredient(std::string name, float amount, bool mass);
		void CreateRecipe(std::string name, std::string instruction, std::string url="nil");
		void MapRecipeToIngredient(std::string recipe, float quantity, std::string ingredient);
		void DeleteIngredient(std::string name);
		void DeleteRecipe(std::string name);
		std::vector<std::pair<int, float>> Reserve(std::vector<std::string> ingredients, std::vector<float> amounts);
		
		void UpdateIngredient(std::string name, float amount);
		std::vector<std::string> GetIngredients(std::string recipe);
		std::vector<std::string> GetRecipes(std::vector<std::string> ingredients);
		std::vector<std::string> GetInstructions(std::vector<std::string> recipes);


		std::string GetIngredientByIndex(int index);
		void Release(std::vector<std::string> items, std::vector<float> amounts);


};
