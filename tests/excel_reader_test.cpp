#include <iostream>
#include <data/excel_reader.h>
#include <models/csv.h>

void excel_reader_test() {
    const std::string filename1 = "../include/data/cvs_data/Table1 - Maintenance Type A Cost.csv";
    const std::string filename2 = "../include/data/cvs_data/Table1 - OD pairs & Reassignment penalty.csv";
    const std::string filename3 = "../include/data/cvs_data/Table1 - Results.csv";

    try {
        std::vector<OD_pairs_and_Penalties> od_pairs;
        csv_to_Od_pairs( filename2, od_pairs);

        Maintenance main;
        csv_to_Maintenance( filename1, main);

        std::vector<Result> results;
        csv_to_Result( filename3, results);

        std::cout << "Table1 - OD pairs & Reassignment penalty" << std::endl;
        for (const auto& od : od_pairs) {
            std::cout << "Origin: " << od.origin_city_id
                      << " Dest: " << od.destination_city_id
                      << " Day: " << od.day_of_week << " Penalties: ";
            for (int p : od.penalties) std::cout << p << " ";
            std::cout << "\n";
        }

        std::cout<< "Table1 - Maintenance Type A Cost" << std::endl;
        std::cout << "Cities: ";
        for ( const auto& city : main.cities ) std::cout << city << " | ";
        std::cout << "\nCost Matrix (" << main.cost.size() << " rows):\n";
        for ( size_t i = 0; i < main.cost.size(); i++ ) {
            std::cout << "  Aircraft " << (i + 1) << ": ";
            for ( int val : main.cost[i] ) std::cout << val << " ";
            std::cout << "\n";
        }
        std::cout << "Capacities: ";
        for ( int cap : main.capacities ) std::cout << cap << " ";
        std::cout << "\n";

        std:: cout << "Table1 - Results" << std::endl;
        for ( const auto& r : results ) {
            std::cout << "Aircraft: " << r.aircraft_id << "\n"
                      << " Main Cost: " << r.weekly_cost_maintenance << "\n"
                      << " Reassign Cost: " << r.weekly_reassignment_cost << "\n"
                      << " Total: " << r.total_cost << "\n";
        }
    }
    catch ( const std::exception& e ) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

int main() {
    std::cout << "test running" << std::endl;
    excel_reader_test();
    std::cout << "test ended" << std::endl;
}