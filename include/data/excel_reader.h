#ifndef AIRPORT_EXEL_READER_H
#define AIRPORT_EXEL_READER_H

#include <string>
#include "../models/csv.h"

void csv_to_Od_pairs( const std::string& filename, std::vector<OD_pairs_and_Penalties>& od_list);
void csv_to_Maintenance( const std::string& filename, Maintenance& maintenance);
void csv_to_Result( const std::string& filename, std::vector<Result>& results);

#endif //AIRPORT_EXEL_READER_H