//
// Created by Uliana Trubach on 17.11.2026.
//

#include <iostream>
#include <stdexcept>
#include <vector>
#include "../include/models/csv.h"
#include "../include/models/algorithm.h"
#include "../include/models/json.h"
#include "../include/data/excel_reader.h"
#include "../include/data/json_reader.h"
#include "../include/json.hpp"

int tests() {
    std::cout<< "starting tests of TAP solver algorithm" << std::endl;
    const std::string json_filename = "../Data/NewInstances/med6_p20_h180_r1 (1).json";
    const std::string csv_od_pen_filename = "../include/data/cvs_data/Table1 - OD pairs & Reassignment penalty.csv";
    const std::string csv_maint_filename = "../include/data/cvs_data/Table1 - Maintenance Type A Cost.csv";

    try {
        JsonDataset json_data = read_json_file(json_filename);
        std::vector<OD_pairs_and_Penalties> od_list ;
        Maintenance maintenance;

        csv_to_Od_pairs(csv_od_pen_filename, od_list);
        csv_to_Maintenance(csv_maint_filename, maintenance);

        std::vector<FlightAssignment> schedule = algorithm_tap(json_data, od_list, maintenance);

        std::cout << "Total flights successfully assigned: " << schedule.size()
                  << " out of " << json_data.flights.size() << "\n\n";

        if (schedule.size() < json_data.flights.size()) {
            std::cout << "\nNote: Some flights were unassigned. Check for turnaround time clashes or missing location matches.\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "ERROR during test execution: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\ntest completed successfully\n";
    return 0;
}


int main() {
    tests();
}