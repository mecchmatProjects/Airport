//
// Created by Uliana Trubach on 19.11.2026.
//

#ifndef AIRPORT_GANTT_H
#define AIRPORT_GANTT_H

#include <iosfwd>
#include<vector>
#include<string>
#include "../include/models/algorithm.h"

void print_gantt_ascii(const std::vector<FlightAssignment>& schedule, std::ostream& output);
bool write_gantt_mermaid(const std::vector<FlightAssignment>& schedule, const std::string& filename);
bool write_gantt_svg(const std::vector<FlightAssignment>& schedule, const std::string& filename);

#endif //AIRPORT_GANTT_H
