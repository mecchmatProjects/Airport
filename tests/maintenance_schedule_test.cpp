#include <cassert>
#include <string>
#include <vector>
#include "../include/models/algorithm.h"

namespace {
JsonDataset make_dataset() {
    JsonDataset data{};
    data.parameters.Target_Horizon_Days = 2;
    data.maintenance_thresholds = {60, 10000, 1000, 1000};
    data.maintenance_durations = {30, 30, 30, 30};
    data.station_capacity.capacities = {{"A", 1}, {"B", 1}};
    data.initial_checks.A = {{"0", 0}, {"1", 0}};
    data.initial_checks.B = {{"0", 0}, {"1", 0}};
    data.initial_checks.C = {{"0", 0}, {"1", 0}};
    data.initial_checks.D = {{"0", 0}, {"1", 0}};
    data.aircraft = {{"0", "A"}, {"1", "A"}};
    return data;
}
}

int main() {
    JsonDataset data = make_dataset();
    std::vector<FlightAssignment> flights;
    for (const std::string aircraft_id : {"0", "1"}) {
        const int offset = aircraft_id == "0" ? 0 : 1;
        const int first_id = 10 + offset;
        const int second_id = 20 + offset;
        data.flights.push_back({first_id, "A", "B", 60, 120});
        data.flights.push_back({second_id, "B", "A", 240, 300});
        flights.push_back({aircraft_id, first_id, 60, 120, 1, "A"});
        flights.push_back({aircraft_id, second_id, 240, 300, 1, "B"});
    }

    const std::vector<MaintenanceAssignment> planned = plan_maintenance(data, flights);
    assert(planned.size() == 4);
    for (const auto& event : planned) {
        assert(event.check_type == 'A');
        assert(event.scheduled);
        assert(event.maintenance_end - event.maintenance_start == 30);
    }
    assert(planned[0].maintenance_start == 120);
    assert(planned[1].maintenance_start == 150);
    assert(planned[2].maintenance_start == 300);
    assert(planned[3].maintenance_start == 330);

    JsonDataset no_station_data = make_dataset();
    no_station_data.aircraft.resize(1);
    no_station_data.initial_checks.A.erase("1");
    no_station_data.initial_checks.B.erase("1");
    no_station_data.initial_checks.C.erase("1");
    no_station_data.initial_checks.D.erase("1");
    no_station_data.station_capacity.capacities["B"] = 0;
    no_station_data.flights.push_back({1, "A", "B", 60, 120});
    const std::vector<FlightAssignment> one_flight = {
        {"0", 1, 60, 120, 1, "A"}
    };
    const std::vector<MaintenanceAssignment> blocked =
        plan_maintenance(no_station_data, one_flight);
    assert(blocked.size() == 1);
    assert(!blocked.front().scheduled);

    JsonDataset calendar_data = make_dataset();
    calendar_data.aircraft.resize(1);
    calendar_data.initial_checks.A.erase("1");
    calendar_data.initial_checks.B.erase("1");
    calendar_data.initial_checks.C.erase("1");
    calendar_data.initial_checks.D.erase("1");
    calendar_data.maintenance_thresholds.C = 1;
    const std::vector<MaintenanceAssignment> calendar_events =
        plan_maintenance(calendar_data, {});
    assert(calendar_events.size() == 1);
    assert(calendar_events.front().check_type == 'C');
    assert(calendar_events.front().scheduled);
    assert(calendar_events.front().maintenance_start == 1440);
}
