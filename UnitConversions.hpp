#include "Enums.hpp"
#include <stdexcept>

// Errors/Exceptions

class mass_volume_conversion_error: public std::exception {
	private:
		char * message;
	public:
		mass_volume_conversion_error(char * msg): message(msg) {};
		char * what(){
			return "arbitrary conversion beteen mass and volume";
		}
};

class unknown_unit_error: public std::exception {
	public:
		char * what(){
			return "unit not included in system";
		}
};

// Volume

float ImperialToImperialVolume(float quantity, units_imperial initial, units_imperial desired){
	switch (initial){
		case fluid_ounce:
			switch(desired){
				case fluid_ounce:
					return quantity;
				case cups:
					return quantity * 0.123223;
				case tablespoons:
					return quantity * 2.;
				case teaspoons:
					return quantity * 6.;
				default:
					throw mass_volume_conversion_error("Conversion from fluid ounce to a mass unit.");
			}
		case cups:
			switch(desired){
				case fluid_ounce:
					return quantity * 8.;
				case cups:
					return quantity * 8.11537;
				case tablespoons:
					return quantity * 16.2307;
				case teaspoons:
					return quantity * 48.6922;
				default:
					throw mass_volume_conversion_error("Conversion from cups to a mass unit.");
			}
		case tablespoons:
			switch(desired){
				case fluid_ounce:
					return quantity * 0.5;
				case cups:
					return quantity * 0.0616115;
				case tablespoons:
					return quantity;
				case teaspoons:
					return quantity * 2.9999999999987765;
				default:
					throw mass_volume_conversion_error("Conversion from tablespoons to a mass unit.");
			}
		case teaspoons:
			switch(desired){
				case fluid_ounce:
					return quantity * 0.166667;
				case cups:
					return quantity * 0.0205372;
				case tablespoons:
					return quantity * 0.333333;
				case teaspoons:
					return quantity;
				default:
					throw mass_volume_conversion_error("Conversion from teaspoons to a mass unit.");
			}
		default:
			throw mass_volume_conversion_error("Initial unit is of mass.");
	}
}

float MetricToMetricVolume(float quantity, units_metric initial, units_metric desired){
	switch(initial){
		case litres:
			switch(desired){
				case litres:
					return quantity;
				case millilitres:
					return quantity * 1000.;
				default:
					throw mass_volume_conversion_error("Conversion from litres to mass unit");
			}
			case millilitres:
			switch(desired){
				case litres:
					return quantity / 1000.;
				case millilitres:
					return quantity;
				default:
					throw mass_volume_conversion_error("Conversion from millilitres to mass unit");
			}
			default:
				throw mass_volume_conversion_error("Conversion from mass to volume unit");
	}
}

float MetricToImperialVolume(float quantity, units_metric initial, units_imperial desired){
	try{
		return ImperialToImperialVolume(
				MetricToMetricVolume(
					quantity, 
					initial,
				   	litres) 
				* 0.236588,
				cups,
			   	desired);
	} catch (mass_volume_conversion_error e){
		return quantity;
	} 
}

float ImperialToMetricVolume(float quantity, units_imperial initial, units_metric desired){
	try{
		return ImperialToImperialVolume(
				MetricToMetricVolume(quantity, initial, litres) * 4.22675,
				cups,
				desired);
	}catch (mass_volume_conversion_error e){
		return quantity;
	}
}

// Mass

float ImperialToImperialMass(float quantity, units_imperial initial, units_imperial desired){
	switch (initial) {
		case ounce:
			switch (desired){
				case ounce:
					return quantity;
				case pounds:
					return quantity * 0.0625;
				default:
					throw mass_volume_conversion_error("Conversion from ounce to volume unit");
			}
		case pounds:
			switch (desired){
				case ounce:
					return quantity * 16.;
				case pounds:
					return quantity;
				default:
					throw mass_volume_conversion_error("Conversion from mass to volume unit");
			}
		default:
			throw mass_volume_conversion_error("Volume unit to mass conversion.");
	}
}

float MetricToMetricMass(float quantity, units_metric initial, units_metric desired){
	switch (desired){
		case kilograms:
			switch(desired){
				case kilograms:
					return quantity;
				case grams:
					return quantity * 1000.;
				default:
					throw mass_volume_conversion_error("Conversion from kilograms to volume unit");
			}
		case grams:
			switch(desired){
				case kilograms:
					return quantity * 0.0001;
				case grams:
					return quantity;
				default:
					throw mass_volume_conversion_error("Conversion from grams to volume unit");
			}
		default:
			throw mass_volume_conversion_error("Conversion from volume unit to mass unit");
	}
}

float MetricToImperialMass(float quantity, units_metric initial, units_imperial desired){
	try{
		ImperialToImperialMass(
				MetricToMetricMass(quantity, inintial, grams) * 0.035274,
				ounces,
				desired
				);

	} catch (mass_volume_conversion_error e){
		return quantity;
	}
}

float ImperialToMetricMass(float quantity, units_imperial initial, units_metric desired){
	try{
		return MetricToMetricMass(
				ImperialToImperialMass(quantity, initial, ounces) * 28.3495,
				grams
				desired
				);
	} catch (mass_volume_conversion_error e){
		return quantity;
	}
}

