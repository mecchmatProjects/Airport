#include <fstream>
#include <stdexcept>

#include <data/json_reader.h>
#include <models/json.h>
#include <json.hpp>
using json = nlohmann::json;

void json_to_Data( const json& j, Data& d ) {
    d.filename = j.at("Original_Filename").get<std::string>();
}

void json_to_Parameters( const json& j, Parameters& params ) {
    params.Input_Horizon_Days = j.at("Input_Horizon_Days").get<int>();
    params.Min_Turn_Minutes = j.at("Min_Turn_Minutes").get<int>();
    params.Target_Horizon_Days = j.at("Target_Horizon_Days").get<int>();
}

void json_to_Aircraft ( const json& j, std::vector<Aircraft>& aircraft_list ) {
    for ( const auto& i : j.items() ) {
        Aircraft aircraft;
        aircraft.id = i.key();
        aircraft.aircraft_init_pos = i.value().get<std::string>();
        aircraft_list.push_back( aircraft );
    }
};

void json_to_Flights ( const json& j, std::vector<Flights>& flights_list ) {
    for ( const auto& i : j ) {
        Flights flight;
        flight.flight_id    = i.at(0).get<int>();
        flight.init_pos     = i.at(1).get<std::string>();
        flight.dest_pos     = i.at(2).get<std::string>();
        flight.start_number = i.at(3).get<int>();
        flight.end_number   = i.at(4).get<int>();
        flights_list.push_back( flight );
    }
};

void json_to_CostMatrix ( const json& j, Cost_matrix& m ) {
    m.matrix = j.get<std::vector<std::vector<double>>>();
}

void json_to_Maintenance_Threshold( const json& j, Maintenance_Thresholds& m) {
    m.A = j.at("A").get<int>();
    m.B = j.at("B").get<int>();
    m.C = j.at("C").get<int>();
    m.D = j.at("D").get<int>();
}

void json_to_Maintenance_Durations( const json& j, Maintenance_Durations& m) {
    m.A = j.at("A").get<int>();
    m.B = j.at("B").get<int>();
    m.C = j.at("C").get<int>();
    m.D = j.at("D").get<int>();
}

void json_to_StationCapacity( const json& j, Station_Capacity& s ) {
    s.capacities = j.get<std::map<std::string, int>>();
}
void json_to_InitialChecks( const json& j, Initial_Checks& i ) {
    i.A = j.at("A").get<std::map<std::string, int>>();
    i.B = j.at("B").get<std::map<std::string, int>>();
    i.C = j.at("C_Days").get<std::map<std::string, int>>();
    i.D = j.at("D_Days").get<std::map<std::string, int>>();
}

void json_to_Units ( const json& j, Units& units ) {
    units.A_B_counters = j.at("A_B_counters").get<std::string>();
    units.C_D_counters = j.at("C_D_counters").get<std::string>();
    units.time = j.at("time").get<std::string>();
}

void json_to_Configuration ( const json& j, Configuration& config ) {
    config.family = j.at("family").get<std::string>();
    config.fleet_size = j.at("fleet_size").get<int>();
    config.n_stations = j.at("n_stations").get<int>();
    config.horizon_days = j.at("horizon_days").get<int>();
    config.legs_per_tail_day = j.at("legs_per_tail_day").get<double>();
    config.capable_fraction = j.at("capable_fraction").get<double>();
    config.usage_tightness = j.at("usage_tightness").get<double>();
    config.cost_dispersion = j.at("cost_dispersion").get<double>();
    config.min_turn_minutes = j.at("min_turn_minutes").get<int>();
    config.root_seed = j.at("root_seed").get<uint64_t>();
}

void json_to_Generator ( const json& j, Generator& gen ) {
    gen.name = j.at("name").get<std::string>();
    gen.version = j.at("version").get<std::string>();
    gen.replicate = j.at("replicate").get<int>();
    json_to_Configuration(j.at("configuration"), gen.config);
    gen.sha256 = j.at("sha256").get<std::string>();
}

void json_to_Metrics ( const json& j, Metric& metric ) {
    metric.n_legs = j.at( "n_legs" ).get<size_t>();
    metric.fleet_size = j.at( "fleet_size" ).get<size_t>();
}

void json_to_Validation ( const json& j, Validation& valid ) {
    valid.feasibility_status = j.at( "feasibility_status" ).get<std::string>();
    valid.errors = j.at( "errors" ).get<std::vector<std::string>>();
    valid.warnings = j.at( "warnings" ).get<std::vector<std::string>>();
    json_to_Metrics(j.at("metrics"), valid.metrics);
}

JsonDataset read_json_file( const std::string& filename ) {

    std::ifstream json_file( filename );
    if ( !json_file.is_open() ) {
        throw std::runtime_error("Unable to open file");
    }
    json data;
    json_file >> data;

    JsonDataset d;
    json_to_Data( data, d.filename );
    json_to_Parameters( data.at( "Parameters" ), d.parameters );
    json_to_Aircraft( data.at( "AIRCRAFT_INIT_POS" ), d.aircraft );
    json_to_Flights( data.at( "Flights" ), d.flights );
    json_to_CostMatrix( data.at( "Cost_Matrix" ), d.cost_maintenance );
    json_to_Maintenance_Threshold(data.at("Maintenance_Thresholds"), d.maintenance_thresholds);
    json_to_Maintenance_Durations(data.at("Maintenance_Durations"), d.maintenance_durations);
    json_to_StationCapacity(data.at("Station_Capacity"), d.station_capacity);
    json_to_InitialChecks(data.at("Initial_Checks"), d.initial_checks);
    json_to_Units ( data.at("Units"), d.units );
    json_to_Generator( data.at("Generator"), d.generator );
    json_to_Validation( data.at("Validation"), d.validation );

    return d;
}