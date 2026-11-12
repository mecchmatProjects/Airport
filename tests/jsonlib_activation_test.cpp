#include <iostream>
#include </Users/chocolatechip/CLionProjects/untitled/Airport/include/json.hpp>
using json = nlohmann::json;

int main() {
    json data;
    data = {
        {"name", "Airport"},
        {"fleet_size", 60},
        {"feasibility_status", "PASS"}
    };

    std::cout << data.dump(4) << std::endl;

    std::cout << "\nLibrary works!" << std::endl;

    return 0;
}