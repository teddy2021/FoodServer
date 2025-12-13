#pragma once

#include <string>
#include <vector>

class IDB{
	public:
		virtual void CreateIngredient( 
				std::string name, 
				float amount, 
				bool mass) = 0;

		virtual void CreateRecipe(std::string name, 
				std::string instruction, 
				std::string url="nil") = 0;
		
		virtual void MapRecipeToIngredient(
				std::string recipe,
				float quantity,
				std::string ingredient
				) = 0;

		virtual void DeleteIngredient(std::string name) = 0;

		virtual void DeleteRecipe(std::string name) = 0;

		virtual std::vector<std::pair<int, float>>  Reserve(
				std::vector<std::string> ingredients,
				std::vector<float> amounts
				) = 0;
		virtual void UpdateIngredient(
				std::string name,
				float amount
				) = 0;

		virtual std::vector <std::string> GetRecipes(std::vector<std::string> ingredients) = 0;

		virtual std::string GetIngredientByIndex(int index) = 0;

		virtual std::vector<std::string> GetIngredients(std::string recipe) = 0;

		virtual std::vector<std::string> GetInstructions(std::vector<std::string> recipes) = 0;

		virtual void Release(
				std::vector<std::string> items,
				std::vector<float> amounts) = 0;

		virtual ~IDB() = default;
};
