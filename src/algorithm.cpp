//
// Created by Uliana Trubach on 14.11.2026.
//

#include <iostream>
#include <map>
#include <algorithm>
#include <stdexcept>
#include "../include/models/algorithm.h"
#include "../include/data/json_reader.h"
#include "../include/models/json.h"
#include "../include/data/excel_reader.h"
#include "../include/models/csv.h"

struct AircraftState {
    std::string aircraft_id;
    std::string current_location;
    int av_time;
    int hours_since_maintenance;
    bool maintenance = false;
};

int get_penalty_for_flight(const std::string& origin,
                           const std::string& destination,
                           int day,
                           const std::vector<OD_pairs_and_Penalties>& od_list) {
    for (const auto& od : od_list) {
        if (std::to_string(od.origin_city_id) == origin &&
            std::to_string(od.destination_city_id) == destination &&
            od.day_of_week == day) {
            if (!od.penalties.empty()) {
                return od.penalties[0];
            }
        }
    }
    return 0;
}

int get_day(const std::vector<OD_pairs_and_Penalties>& od_list) {
    if (od_list.empty()) return 1;
    return od_list.at(0).day_of_week;
}

void get_data_json(const char* filename) {
    JsonDataset data = read_json_file(filename);
}

void get_data_csv(const std::string& od_filename, const std::string& maint_filename) {
    std::vector<OD_pairs_and_Penalties> od_list;
    Maintenance maintenance_data;
    try {
        csv_to_Od_pairs(od_filename, od_list);
        csv_to_Maintenance(maint_filename, maintenance_data);
        std::cout << "Successfully loaded " << od_list.size() << " OD pairs and maintenance data!\n";
    } catch (const std::exception& e) {
        std::cerr << "Error loading CSV files: " << e.what() << "\n";
    }
}

std::vector<FlightAssignment> algorithm_tap(JsonDataset data,
                                            const std::vector<OD_pairs_and_Penalties>& od_list,
                                            const Maintenance& maintenance) {
    std::vector<FlightAssignment> schedule;

    auto flights = data.flights;
    auto aircrafts = data.aircraft;

    std::sort(flights.begin(), flights.end(), [](const Flights& a, const Flights& b) {
        return a.start_number < b.start_number;
    });

    std::vector<AircraftState> fleet_states;
    for (const auto& ac : aircrafts) {
        fleet_states.push_back({ac.id, ac.aircraft_init_pos, 0, 0});
    }

    for (const auto& flight : flights) {
        AircraftState* best_aircraft = nullptr;
        int min_cost = 1e9;

        for (auto& ac : fleet_states) {
            if (ac.maintenance) continue;
            const bool location = ac.current_location == flight.init_pos;
            const bool time = ac.av_time <= flight.start_number - 30;
            if (location && time) {
                const int base_cost = 10;
                const int day = od_list.empty() ? 1 : od_list.at(0).day_of_week;
                const int penalty = get_penalty_for_flight(flight.init_pos, flight.dest_pos, day, od_list);
                const int total_cost = base_cost + penalty;
                if (total_cost < min_cost) {
                    min_cost = total_cost;
                    best_aircraft = &ac;
                }
            }
        }

        if (best_aircraft != nullptr) {
            FlightAssignment assignment;
            assignment.aircraft_id = best_aircraft->aircraft_id;
            assignment.flight_id = flight.flight_id;
            assignment.flight_start = flight.start_number;
            assignment.flight_end = flight.end_number;
            assignment.day_of_week = get_day(od_list);
            assignment.assigned_airport = flight.init_pos;

            schedule.push_back(assignment);

            best_aircraft->current_location = flight.dest_pos;
            best_aircraft->av_time = flight.end_number;
        }
    }
    return schedule;
}
