
#pragma once
#include <string>

enum	protocol_type		{tcp, udp};

enum 	buffer_state 		{clean, in_use, storing, dirty};
constexpr std::string_view 	buffer_state_text[] = {"clean", "in_use", "storing", "dirty"};

enum 	units 				{kilograms, grams, litres, millilitres,ounce, fluid_ounce, cups, tablespoons, teaspoons, pounds, pinch};

enum 	requests 			{
	syn = 0, 
	reserve=1, 
	release=2, 
	finalize=3, 
	scrape=4, 
	recipe=5, 
	match = 6,
	stop = 7,
	aingredient = 10,
	arecipe = 11,
	ringredient = 20,
	rrecipe = 21
};
constexpr 	std::string_view 	request_text[] = { "syn", "reserve",
   	"release", "finalize", "scrape", "recipe", "match", 
	"stop", " ", " ",  "add ingredient", "add recipe", 
" ", " ", " ", " ", " ", " ", " ", " ",
 "remove ingredient", "remove recipe"};

class invalid_state_exception: public std::exception{
	private:
		std::string message;
	public:
		std::string what(){
			return message;
		};
		invalid_state_exception(std::string msg): message(msg){};
};
