//
// Created by Uliana Trubach on 19.11.2026.
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
#include "../include/models/gantt.h"
int tests() {
    std::cout<< "starting tests of Gantt diagram visualisation" << std::endl;
    const std::string json_filename = "Data/NewInstances/med6_p20_h180_r1 (1).json";
    const std::string csv_od_pen_filename = "include/data/cvs_data/Table1 - OD pairs & Reassignment penalty.csv";
    const std::string csv_maint_filename = "include/data/cvs_data/Table1 - Maintenance Type A Cost.csv";

    try {
        JsonDataset json_data = read_json_file(json_filename);
        std::vector<OD_pairs_and_Penalties> od_list ;
        Maintenance maintenance;

        csv_to_Od_pairs(csv_od_pen_filename, od_list);
        csv_to_Maintenance(csv_maint_filename, maintenance);

        std::vector<FlightAssignment> schedule = algorithm_tap(json_data, od_list, maintenance);

        std::cout << "\n=== Test Results ===\n";

        print_gantt_ascii(schedule, std::cout);
        if (!write_gantt_mermaid(schedule, "schedule.mmd")) {
            throw std::runtime_error("Unable to write schedule.mmd");
        }
        if (!write_gantt_svg(schedule, "schedule.svg")) {
            throw std::runtime_error("Unable to write schedule.svg");
        }
        if (!write_gantt_html(schedule, "schedule.html")) {
            throw std::runtime_error("Unable to write schedule.html");
        }
        std::cout << "\nGantt outputs written to schedule.html, schedule.svg and schedule.mmd"
                  << "\nOpen schedule.html for the interactive daily view.\n";
    }
    catch (std::runtime_error& e) {
        std::cout << "\nError: " << e.what() << std::endl;
    }
    std::cout<<"\ntest completed successfully"<<std::endl;
}

int main() {
    tests();
    return 0;
}