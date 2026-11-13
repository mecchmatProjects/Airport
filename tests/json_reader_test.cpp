#include <iostream>
#include <exception>
#include <data/json_reader.h>

int main() {
    const std::string filepath = "Airport/Data/NewInstances/small_p8_h60_r1.json";

    try {
        JsonDataset dataset = read_json_file(filepath);

        std::cout << "Original Filename : " << dataset.filename.filename << "\n";
        std::cout << "Input Horizon     : " << dataset.parameters.Input_Horizon_Days << "\n";
        std::cout << "Aircraft Count    : " << dataset.aircraft.size() << "\n";
        std::cout << "Flights Count     : " << dataset.flights.size() << "\n";
        std::cout << "Cost Matrix Rows  : " << dataset.cost_maintenance.matrix.size() << "\n";
        std::cout << "Threshold A       : " << dataset.maintenance_thresholds.A << "\n";
        std::cout << "Station A Cap     : " << dataset.station_capacity.capacities.at("A") << "\n";
        std::cout << "Generator Name    : " << dataset.generator.name << "\n";
        std::cout << "Feasibility       : " << dataset.validation.feasibility_status << "\n";

        if (!dataset.aircraft.empty()) {
            std::cout << "First Aircraft    : ID=" << dataset.aircraft[0].id // only first
                      << ", Pos=" << dataset.aircraft[0].aircraft_init_pos << "\n";
        }
        if (!dataset.flights.empty()) {
            std::cout << "First Flight      : ID=" << dataset.flights[0].flight_id // only first
                      << ", From=" << dataset.flights[0].init_pos << "\n";
        }

    } catch (const nlohmann::json::exception& e) { // json error
        std::cerr << "JSON PARSE ERROR " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) { // other errors
        std::cerr << "ERROR " << e.what() << "\n";
        return 1;
    }

    return 0;
}
