//
// Created by Uliana Trubach on 14.11.2026.
//

#ifndef AIRPORT_ALGORYTHM_H
#define AIRPORT_ALGORYTHM_H
#include <iostream>
#include <string>
#include <vector>
#include "json.h"
#include "csv.h"

struct FlightAssignment {
    std::string aircraft_id;
    int flight_id;
    int flight_start;
    int flight_end;
    int day_of_week;
    std::string assigned_airport;
};
int get_penalty_for_flight(const std::string& origin,
                           const std::string& destination,
                           int day,
                           const std::vector<OD_pairs_and_Penalties>& od_list);
int get_day(const std::vector<OD_pairs_and_Penalties>& od_list, const Flights &flight);
JsonDataset get_data_json(const std::string& filename);
void get_data_csv(const std::string& od_filename, const std::string& maint_filename);
std::vector<FlightAssignment> algorithm_tap(JsonDataset data, const std::vector<OD_pairs_and_Penalties>& od_list, const Maintenance& maintenance);
void print_gantt_ascii(const std::vector<FlightAssignment>& schedule, std::ostream& output = std::cout);
bool write_gantt_mermaid(const std::vector<FlightAssignment>& schedule, const std::string& filename);
bool write_gantt_svg(const std::vector<FlightAssignment>& schedule, const std::string& filename);
bool write_gantt_html(const std::vector<FlightAssignment>& schedule, const std::string& filename);

#endif //AIRPORT_ALGORYTHM_H
