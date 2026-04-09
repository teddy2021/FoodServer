
#pragma once
#include <optional>
#include <string>
#include <vector>

struct RecipeData{
    std::string name;
    std::vector<std::string> ingredients;
    std::string instructions;
};

class WebScraper{

    public:
        
        RecipeData data;
        
        std::optional<RecipeData> Scrape(const std::string& url);
};
