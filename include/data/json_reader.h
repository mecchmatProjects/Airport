#ifndef AIRPORT_JSON_READER_H
#define AIRPORT_JSON_READER_H

#include <json.hpp>
#include <models/json.h>
using json = nlohmann::json;

JsonDataset read_json_file(const std::string& filename);

void json_to_Data( const json& j, Data& d );
void json_to_Parameters( const json& j, Parameters& params );
void json_to_Aircraft ( const json& j, std::vector<Aircraft>& aircraft_list );
void json_to_Flights ( const json& j, std::vector<Flights>& flights_list );
void json_to_CostMatrix ( const json& j, Cost_matrix& m );
void json_to_Maintenance_Threshold( const json& j, Maintenance_Thresholds& m);
void json_to_Maintenance_Durations( const json& j, Maintenance_Durations& m);
void json_to_StationCapacity( const json& j, Station_Capacity& s );
void json_to_InitialChecks( const json& j, Initial_Checks& i );
void json_to_Units ( const json& j, Units& units );
void json_to_Configuration ( const json& j, Configuration& config );
void json_to_Metrics ( const json& j, Metric& metric );
void json_to_Generator ( const json& j, Generator& gen );
void json_to_Metrics ( const json& j, Metric& metric );
void json_to_Validation ( const json& j, Validation& valid );

#endif //AIRPORT_JSON_READER_H