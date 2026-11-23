#include <fstream>
#include <iostream>
#include <sstream>
#include "data/excel_reader.h"
#include "models/csv.h"

void csv_to_Od_pairs( const std::string& filename, std::vector<OD_pairs_and_Penalties>& od_list) {
    std::ifstream file( filename );
    if ( !file.is_open() ) {
        throw std::runtime_error("Unable to open file");
    }
    std::string line;
    std::getline( file, line ); // skip the header
    while ( std::getline( file, line ) ) {
        if (line.empty()) continue;
        std::stringstream ss;
        ss << line;

        std:: string cell;
        std::vector<std::string> row;
        while (std::getline( ss, cell, ',' ) ) {
            if ( !cell.empty() && cell.back() == '\r' ) {
                cell.pop_back();
            }
            row.push_back( cell );
        }
        if (row.size() < 3 || row[0].empty() || !std::isdigit(row[0][0])) {
            continue;
        }


        OD_pairs_and_Penalties od;
        od.origin_city_id = std::stoi(row[0]); // convert from string to int
        od.destination_city_id = std::stoi(row[1]);
        od.day_of_week = std::stoi(row[2]);

        for ( size_t i = 3; i < row.size(); i++ ) {
            if ( row[i].find_first_of("0123456789") != std::string::npos ) { // ???
                od.penalties.push_back(std::stoi(row[i]));
            }
        }
        od_list.push_back(od);
    }
}

void csv_to_Maintenance( const std::string& filename, Maintenance& main ) {
    std::ifstream file( filename );
    if ( !file.is_open() ) {
        throw std::runtime_error("Unable to open file");
    }
    std::string line;
    if ( std::getline(file, line) ) {
        std::stringstream ss(line);
        std::string cell;
        std::getline(ss, cell, ','); // skip the first cell
        while ( std::getline(ss, cell, ',') ) { // fill cities in struct( first line of table )
            if ( !cell.empty() && cell.back() == '\r' ) {
                cell.pop_back();
            }
            if ( !cell.empty() ) {
                main.cities.push_back(cell);
            }
        }
    }
    while ( std::getline( file, line ) ) { // other rows
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> row;
        while (std::getline( ss, cell, ',' ) ) {
            if ( !cell.empty() && cell.back() == '\r' ) {
                cell.pop_back();
            }
            row.push_back( cell );
        }
        if (row.size() < 2 || row[0].empty() || (row[0] != "Capacity" && !std::isdigit(row[0][0]))) {
            continue;
        }

        std::vector<int> rest;
        for (size_t i = 1; i < row.size(); ++i) {
            if ( row[i].find_first_of("0123456789") != std::string::npos ) { // ???
                rest.push_back(std::stoi(row[i]));
            }
        }
        if ( row[0] == "Capacity" ) {
            main.capacities =  rest;
        } else {
            main.aircraft_id = std::stoi( row[0] );
            main.cost.push_back( rest );
        }
    }
}

void csv_to_Result( const std::string& filename, std::vector<Result>& res_list) {
    std::ifstream file( filename );
    if ( !file.is_open() ) {
        throw std::runtime_error("Unable to open file");
    }
    std::string line;
    std::getline( file, line ); // skip the header
    while ( std::getline( file, line ) ) {
        if (line.empty()) continue;
        for (char& c : line) {
            if (c == '"' || c == '[' || c == ']' || c == '\r') {
                c = ' ';
            }
        }
        std::stringstream ss(line);
        std:: string cell;
        std::vector<std::string> row;
        while (std::getline( ss, cell, ',' ) ) {
            if ( !cell.empty() && cell.back() == '\r' ) {
                cell.pop_back();
            }
            row.push_back( cell );
        }
        if (row.size() < 6 || row[0].empty() || !std::isdigit(row[0][0]) ||
            row[1].find_first_of("0123456789") == std::string::npos || // whyyyyyy?
            row[2].find_first_of("0123456789") == std::string::npos) {
            continue;
            }
        Result res;
        res.aircraft_id = std::stoi( row[0] );
        res.maintenance_days.push_back({ std::stoi(row[1]), std::stoi(row[2]) });
        res.weekly_cost_maintenance = std::stoi( row[3] );
        res.weekly_reassignment_cost = std::stoi( row[4] );
        res.total_cost = std::stoi( row[5] );

        res_list.push_back(res);
    }
}