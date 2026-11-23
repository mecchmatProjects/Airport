//
// Created by Uliana Trubach on 17.11.2026.
//

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cassert>
#include <algorithm>
#include "../include/models/algorithm.h"
#include "../include/models/json.h"
#include "../include/models/csv.h"
#include "../include/data/excel_reader.h"
#include "../include/data/json_reader.h"
#include "../include/json.hpp"

int tests() {
    std::cout<< "starting tests of TAP solver algorithm" << std::endl;
    const std::string json_filename = "../Data/NewInstances/lge1_p60_h180_r1.json";
    const std::string csv_od_pen_filename = "../include/data/cvs_data/Table1 - OD pairs & Reassignment penalty.csv";
    const std::string csv_maint_filename = "../include/data/cvs_data/Table1 - Maintenance Type A Cost.csv";

    try {
        JsonDataset json_data = read_json_file(json_filename);
        std::vector<OD_pairs_and_Penalties> od_list ;
        Maintenance maintenance;

        csv_to_Od_pairs(csv_od_pen_filename, od_list);
        csv_to_Maintenance(csv_maint_filename, maintenance);

        std::vector<FlightAssignment> schedule = algorithm_tap(json_data, od_list, maintenance);
        std::vector<MaintenanceAssignment> maintenance_schedule =
            plan_maintenance(json_data, schedule);

        std::cout << "\n=== Test Results ===\n";
        std::cout << "Total flights successfully assigned: " << schedule.size()
                  << " out of " << json_data.flights.size() << "\n\n";
        const auto scheduled_maintenance = std::count_if(
            maintenance_schedule.begin(), maintenance_schedule.end(),
            [](const MaintenanceAssignment& item) { return item.scheduled; });
        std::cout << "Maintenance checks scheduled: " << scheduled_maintenance << "\n"
                  << "Maintenance conflicts: " << maintenance_schedule.size() - scheduled_maintenance
                  << "\n\n";

        print_gantt_ascii(schedule);
        if (!write_gantt_mermaid(schedule, "schedule.mmd")) {
            throw std::runtime_error("Unable to write schedule.mmd");
        }
        if (!write_gantt_svg(schedule, "schedule.svg")) {
            throw std::runtime_error("Unable to write schedule.svg");
        }
        if (!write_gantt_html(schedule, "schedule.html", maintenance_schedule,
                              json_data.aircraft, json_data.parameters.Target_Horizon_Days)) {
            throw std::runtime_error("Unable to write schedule.html");
        }
        std::cout << "\nGantt outputs written to schedule.html, schedule.svg and schedule.mmd"
                  << "\nOpen schedule.html for the interactive daily view.\n";

        int print_limit = std::min(10, (int)schedule.size());
        for (int i = 0; i < print_limit; ++i) {
            const auto& assignment = schedule[i];
            std::cout << "Flight ID: " << assignment.flight_id
                      << " ---> Assigned to Aircraft: " << assignment.aircraft_id
                      << " (Start: " << assignment.flight_start
                      << ", End: " << assignment.flight_end << ")\n";
        }

        if (schedule.size() < json_data.flights.size()) {
            std::cout << "\nNote: Some flights were unassigned. Check for turnaround time clashes or missing location matches.\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "ERROR during test execution: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\n=== Test Complete ===\n";
    return 0;
}


int main() {
    return tests();
}