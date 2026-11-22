//
// Created by Uliana Trubach on 22.11.2026.
//

#include <iostream>
#include <stdexcept>
#include <vector>
#include <filesystem>
#include <string>
#include "../include/models/csv.h"
#include "../include/models/algorithm.h"
#include "../include/models/json.h"
#include "../include/data/excel_reader.h"
#include "../include/data/json_reader.h"
#include "../include/json.hpp"
#include "../include/models/gantt.h"

std::vector<std::string> get_file_names(const std::filesystem::path& folder_path, const std::string& ext = "") {
    std::vector<std::string> files;
    if (!std::filesystem::exists(folder_path) || !std::filesystem::is_directory(folder_path)) {
        std::cerr << "Directory not found: " << std::filesystem::weakly_canonical(folder_path) << "\n";
        return files;
    }

    for (const auto& entry : std::filesystem::directory_iterator(folder_path)) {
        if (!entry.is_regular_file()) continue;
        if (ext.empty() || entry.path().extension() == ext) {
            files.push_back(entry.path().string());
        }
    }
    return files;
}

int main() {
    //basic data input//
    std::vector<std::string> json_files = get_file_names("../Data/NewInstances", ".json");

    std::vector<std::string> csv_od_pairs = {"../include/data/cvs_data/Table1 - OD pairs & Reassignment penalty.csv",
        "../include/data/cvs_data/Table 2 - OD pairs & Reassignment penalty.csv",
        "../include/data/cvs_data/Table 3 - OD pairs & Reassignment penalty.csv"};
    std::vector<std::string> csv_maint = {"../include/data/cvs_data/Table1 - Maintenance Type A Cost.csv",
        "../include/data/cvs_data/Table 2 - Maintenance Type A Cost.csv",
        "../include/data/cvs_data/Table 3 - Maintenance Type A Cost.csv"};

    const size_t num_tables = std::min(csv_od_pairs.size(), csv_maint.size());

    for (size_t t = 0; t < num_tables; ++t) {
        std::vector<OD_pairs_and_Penalties> od_table;
        Maintenance maintenance;

        csv_to_Od_pairs(csv_od_pairs[t], od_table);
        csv_to_Maintenance(csv_maint[t], maintenance);

        std::string table_tag = "T" + std::to_string(t + 1);
        std::cout << "Loaded " << table_tag << " OD pairs and maintenance\n";

        for (const auto& file_json : json_files) {
            try {
                std::string stem = std::filesystem::path(file_json).stem().string();
                std::string out_prefix = "schedule_" + stem + "_" + table_tag;
                JsonDataset json_data = read_json_file(file_json);    //check some more this, here is a mistake
                std::vector<FlightAssignment> schedule = algorithm_tap(json_data, od_table, maintenance);

                write_gantt_mermaid(schedule, out_prefix + ".mmd");
                write_gantt_svg(schedule, out_prefix + ".svg");
                write_gantt_html(schedule, out_prefix + ".html");

                std::cout << "schedule for " << stem << " (" << table_tag << ") is complete\n";
            } catch (const std::exception& e) {
                std::cerr << "Error on " << file_json << " (" << table_tag << "): " << e.what() << "\n";
            }
        }
    }
    return 0;
}