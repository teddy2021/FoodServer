#pragma once

#include <map>
#include <algorithm>

#include "../IDB.hpp"

using std::string;
using std::map;
using std::vector;
using std::pair;

struct _grocery {
	float amount;
	bool mass;
};

struct _mapping{
	float amount;
	string ingredient;
};

class mockdb: public IDB {
	private:
		map<string, _grocery> groceries; 
		map<string, string> recipes;
		map<string, vector<_mapping>> mappings;
		map<string, float> reserved;
	public:
		mockdb(){};
		mockdb(string address){};
		
		void CreateIngredient(string name, float amount, bool mass) override{
			_grocery groce = { amount, mass};
			groceries.try_emplace(name, amount);
		}

		void CreateRecipe(string name, string instruction, string url="nil") override{
			recipes.try_emplace(name, instruction);
		}

		void MapRecipeToIngredient(string recipe, float quantity, string ingredient) override{
			auto it1 = recipes.find(recipe);
			auto it2 = groceries.find(ingredient);
			if(it1 != recipes.end() && it2 != groceries.end()){
				_mapping mapping = {quantity, ingredient};
				mappings.try_emplace(recipe, mapping);
			}
		}

		void DeleteIngredient(string name) override{
			auto it = groceries.find(name);
			if(it != groceries.end()){
				groceries.erase(it);
			}
		}

		void DeleteRecipe(string name) override{
			auto it = recipes.find(name);
			if(it != recipes.end()){
				recipes.erase(it);
			}
		}

		vector<pair<int, float>> Reserve(vector<string> ingredients, vector<float> amounts) override{
			vector<pair<int, float>> out;
			for(int i = 0; i < ingredients.size(); i += 1){
				auto it = reserved.find(ingredients[i]);
				if(it != reserved.end()){
					it->second += amounts[i];
				}
				else{
					reserved.emplace(ingredients[i], amounts[i]);
				}
				out.push_back(pair<int, float>(i, amounts[i]));
			}
			return out;
			
		}

		void UpdateIngredient(string name, float amount) override{
			auto it = groceries.find(name);
			if( it != groceries.end()){
				it->second.amount += amount;
			}
		}

		vector<string> GetIngredients(string recipe) override{
			vector<string> out;
			auto it = mappings.find(recipe);
			if(it != mappings.end()){
				vector<_mapping> res = it->second;
				for(int i = 0; i < res.size(); i += 1){
					out.push_back(res[i].ingredient);
				}
			}
			return out;
		}

		vector<string> GetRecipes(vector<string> ingredients) override{
			vector<string> out;
			for(auto it = mappings.begin(); it != mappings.end(); it++){
				vector<_mapping> res = it->second;
				for(int j = 0; j < ingredients.size(); j += 1){
					if(std::find(out.begin(), out.end(), ingredients[j]) == out.end()){
						for(int k = 0; k < res.size(); k += 1){
							if(res[k].ingredient == ingredients[j]){
								out.push_back(ingredients[j]);
								break;
							}
						}
					}
				}
			}
			return out;
		}

		string GetIngredientByIndex(int index) override{
			int i = 0;
			auto it = groceries.begin();
			while(i < index && it != groceries.end()){
				it++;
				i += 1;
			}
			return (it != groceries.end()) ? it->first : "";
		}

		vector<string> GetInstructions(vector<string> recipes) override{
			vector<string> out;
			for(int i = 0; i < recipes.size(); i += 1){
				out.push_back(this->recipes[recipes[i]]);
			}
			return out;
		}

		void Release(vector<string> items, vector<float> amounts) override{
			for(int i = 0; i < items.size(); i += 1){
				auto it = reserved.find(items[i]);
				if(it != reserved.end() && it->second - amounts.at(i) <= 0){
					reserved.erase(it);
					
				} else {
					it->second -= amounts[i];
				}
			}
		}


};
