#ifndef AIRPORT_JSON_H
#define AIRPORT_JSON_H

#include <map>
#include <string>
#include <vector>
#include <cstdint>


struct Data {
    std::string filename;
};

struct Parameters {
    int Input_Horizon_Days;
    int Target_Horizon_Days;
    int Min_Turn_Minutes;
};

struct Aircraft {
    std::string id;
    std::string aircraft_init_pos; // "dictionary" of AIRPORT_INIT_POS
};

struct Flights {
    int flight_id;
    std::string init_pos;
    std::string dest_pos;
    int start_number;
    int end_number;
};

struct Cost_matrix {
    std::vector<std::vector<double>> matrix; // matrix through vector
};

struct Maintenance_Thresholds {
    int A;
    int B;
    int C;
    int D;
};

struct Maintenance_Durations {
    int A;
    int B;
    int C;
    int D;
};

struct Station_Capacity {
    std::map<std::string, int> capacities; // dictionary of capacities
};

struct Initial_Checks {
    std::map<std::string, int> A; // dictionaries for each check
    std::map<std::string, int> B;
    std::map<std::string, int> C;
    std::map<std::string, int> D;
};

struct Units {
    std::string time;
    std::string A_B_counters;
    std::string C_D_counters;
};

struct Configuration {
    std::string family;
    int fleet_size;
    int n_stations;
    int horizon_days;
    int legs_per_tail_day;
    double capable_fraction;
    double usage_tightness;
    double cost_dispersion;
    int min_turn_minutes;
    uint64_t root_seed;
};

struct Generator {
    std::string name;
    std::string version;
    int replicate; // ? data type
    Configuration config;
    std::string sha256;
};

struct Metric {
    size_t n_legs;
    size_t fleet_size;
};

struct Validation {
    std::string feasibility_status;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    Metric metrics;
};

struct JsonDataset {
    Data filename;
    Parameters parameters;
    std::vector<Aircraft> aircraft;
    std::vector<Flights> flights;
    Cost_matrix cost_maintenance;
    Maintenance_Thresholds maintenance_thresholds;
    Maintenance_Durations maintenance_durations;
    Station_Capacity station_capacity;
    Initial_Checks initial_checks;
    Units units;
    Generator generator;
    Validation validation;
};

#endif //AIRPORT_JSON_H