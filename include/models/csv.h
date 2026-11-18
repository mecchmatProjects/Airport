#ifndef UNTITLED_CSV_H
#define UNTITLED_CSV_H
#include <vector>

struct OD_pairs_and_Penalties {
    int origin_city_id;
    int destination_city_id;
    int day_of_week;
    std::vector <int> penalties;
};

struct Maintenance {
    int aircraft_id;
    std::vector <std::string> cities;
    std::vector < std::vector<int> > cost; // matrix of costs above capacities
    std::vector <int> capacities;
};

struct Result { // to run checks
    int aircraft_id;
    std::vector < std::vector<int> > maintenance_days;
    int weekly_cost_maintenance;
    int weekly_reassignment_cost;
    int total_cost;
    std::vector<int> cycle;
};


#endif //UNTITLED_CSV_H