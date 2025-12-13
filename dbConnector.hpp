#pragma once
#include "Enums.hpp"
#include "IDB.hpp"
#include <utility>
#include <mariadb/conncpp.hpp>

class DBConnector: public IDB{

	private:
		sql::Driver * driver;
		std::unique_ptr<sql::Connection> connection;
		
	public:
		// Constructors/Destructors
		DBConnector();
		DBConnector(std::string address);

		// Recipe/Ingredient manipulations
		void CreateIngredient(std::string name, float amount, bool mass) override;
		void CreateRecipe(std::string name, std::string instruction, std::string url="nil") override;
		void MapRecipeToIngredient(std::string recipe, float quantity, std::string ingredient) override;
		void DeleteIngredient(std::string name) override;
		void DeleteRecipe(std::string name) override;
		std::vector<std::pair<int, float>> Reserve(std::vector<std::string> ingredients, std::vector<float> amounts) override;
		
		void UpdateIngredient(std::string name, float amount) override;
		std::vector<std::string> GetIngredients(std::string recipe) override;
		std::vector<std::string> GetRecipes(std::vector<std::string> ingredients) override;
		std::vector<std::string> GetInstructions(std::vector<std::string> recipes) override;

		std::string GetIngredientByIndex(int index) override;
		void Release(std::vector<std::string> items, std::vector<float> amounts) override;


};
