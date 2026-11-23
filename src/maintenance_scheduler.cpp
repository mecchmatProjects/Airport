#include <algorithm>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "../include/models/algorithm.h"

namespace {
struct MaintenanceRequest {
    int due_time;
    int duration;
    int repeat_interval;
    std::string aircraft_id;
    char check_type;
};

struct GroundGap {
    int start;
    int end;
    std::string airport;
};

struct CapacityReservation {
    int start;
    int end;
    std::string airport;
};

struct RequestOrder {
    bool operator()(const MaintenanceRequest& left, const MaintenanceRequest& right) const {
        if (left.due_time != right.due_time) return left.due_time > right.due_time;
        if (left.aircraft_id != right.aircraft_id) return left.aircraft_id > right.aircraft_id;
        return left.check_type > right.check_type;
    }
};

int counter_for_aircraft(const std::map<std::string, int>& counters,
                         const std::string& aircraft_id,
                         char check_type) {
    const auto found = counters.find(aircraft_id);
    if (found == counters.end()) {
        throw std::runtime_error(
            "Missing initial " + std::string(1, check_type) +
            " maintenance counter for aircraft " + aircraft_id);
    }
    return found->second;
}

bool has_station_capacity(const std::string& airport,
                          int capacity,
                          int start,
                          int end,
                          const std::vector<CapacityReservation>& reservations,
                          int& next_available_time) {
    std::vector<int> check_times{start};
    for (const auto& reservation : reservations) {
        if (reservation.airport == airport && reservation.start > start && reservation.start < end) {
            check_times.push_back(reservation.start);
        }
    }
    std::sort(check_times.begin(), check_times.end());
    check_times.erase(std::unique(check_times.begin(), check_times.end()), check_times.end());

    for (const int time : check_times) {
        int active = 0;
        next_available_time = std::numeric_limits<int>::max();
        for (const auto& reservation : reservations) {
            if (reservation.airport == airport &&
                reservation.start <= time && time < reservation.end) {
                ++active;
                next_available_time = std::min(next_available_time, reservation.end);
            }
        }
        if (active >= capacity) return false;
    }
    return true;
}

int find_slot(const std::vector<GroundGap>& gaps,
              int due_time,
              int duration,
              int aircraft_available_time,
              const std::map<std::string, int>& station_capacities,
              const std::vector<CapacityReservation>& reservations,
              std::string& airport) {
    for (const auto& gap : gaps) {
        if (gap.end <= due_time) continue;

        const auto capacity = station_capacities.find(gap.airport);
        if (capacity == station_capacities.end() || capacity->second <= 0) return -1;

        int candidate = std::max({gap.start, due_time, aircraft_available_time});
        while (candidate <= gap.end - duration) {
            int next_available = std::numeric_limits<int>::max();
            if (has_station_capacity(gap.airport, capacity->second, candidate,
                                     candidate + duration, reservations, next_available)) {
                airport = gap.airport;
                return candidate;
            }
            if (next_available <= candidate ||
                next_available == std::numeric_limits<int>::max()) {
                return -1;
            }
            candidate = next_available;
        }
        return -1;
    }
    return -1;
}
}

std::vector<MaintenanceAssignment> plan_maintenance(
    const JsonDataset& data,
    const std::vector<FlightAssignment>& schedule) {
    constexpr int minutes_per_day = 1440;
    int horizon = std::max(0, data.parameters.Target_Horizon_Days) * minutes_per_day;
    for (const auto& assignment : schedule) {
        horizon = std::max(horizon, assignment.flight_end);
    }
    if (horizon == 0) return {};

    std::map<std::string, const Aircraft*> aircraft_by_id;
    for (const auto& aircraft : data.aircraft) {
        aircraft_by_id.emplace(aircraft.id, &aircraft);
    }

    std::map<int, const Flights*> flights_by_id;
    for (const auto& flight : data.flights) {
        flights_by_id.emplace(flight.flight_id, &flight);
    }

    std::map<std::string, std::vector<const FlightAssignment*>> assignments_by_aircraft;
    for (const auto& assignment : schedule) {
        if (aircraft_by_id.find(assignment.aircraft_id) == aircraft_by_id.end()) {
            throw std::runtime_error("Schedule references unknown aircraft " + assignment.aircraft_id);
        }
        if (flights_by_id.find(assignment.flight_id) == flights_by_id.end()) {
            throw std::runtime_error("Schedule references unknown flight " +
                                     std::to_string(assignment.flight_id));
        }
        if (assignment.flight_end <= assignment.flight_start) {
            throw std::runtime_error("Flight assignment has a non-positive duration");
        }
        assignments_by_aircraft[assignment.aircraft_id].push_back(&assignment);
    }

    std::map<std::string, std::vector<GroundGap>> ground_gaps;
    for (const auto& aircraft : data.aircraft) {
        auto& assignments = assignments_by_aircraft[aircraft.id];
        std::sort(assignments.begin(), assignments.end(),
                  [](const FlightAssignment* left, const FlightAssignment* right) {
                      return left->flight_start < right->flight_start;
                  });

        int available_time = 0;
        std::string airport = aircraft.aircraft_init_pos;
        auto& gaps = ground_gaps[aircraft.id];
        for (const auto* assignment : assignments) {
            if (assignment->flight_start < available_time) {
                throw std::runtime_error("Overlapping flights assigned to aircraft " + aircraft.id);
            }
            gaps.push_back({available_time, assignment->flight_start, airport});
            airport = flights_by_id.at(assignment->flight_id)->dest_pos;
            available_time = assignment->flight_end;
        }
        gaps.push_back({available_time, horizon, airport});
    }

    std::priority_queue<MaintenanceRequest, std::vector<MaintenanceRequest>, RequestOrder> requests;
    auto add_flight_hour_requests = [&](char check_type, int threshold, int duration,
                                        const std::map<std::string, int>& initial_counters) {
        if (threshold <= 0 || duration <= 0) {
            throw std::runtime_error("Maintenance thresholds and durations must be positive");
        }
        for (const auto& aircraft : data.aircraft) {
            int counter = counter_for_aircraft(initial_counters, aircraft.id, check_type);
            if (counter >= threshold) {
                requests.push({0, duration, 0, aircraft.id, check_type});
                counter = 0;
            }
            for (const auto* assignment : assignments_by_aircraft[aircraft.id]) {
                counter += assignment->flight_end - assignment->flight_start;
                if (counter >= threshold) {
                    requests.push({assignment->flight_end, duration, 0, aircraft.id, check_type});
                    counter = 0;
                }
            }
        }
    };

    add_flight_hour_requests('A', data.maintenance_thresholds.A,
                             data.maintenance_durations.A, data.initial_checks.A);
    add_flight_hour_requests('B', data.maintenance_thresholds.B,
                             data.maintenance_durations.B, data.initial_checks.B);

    auto add_calendar_request = [&](char check_type, int threshold_days, int duration,
                                    const std::map<std::string, int>& initial_days) {
        if (threshold_days <= 0 || duration <= 0) {
            throw std::runtime_error("Maintenance thresholds and durations must be positive");
        }
        const int interval = threshold_days * minutes_per_day;
        for (const auto& aircraft : data.aircraft) {
            const int initial = counter_for_aircraft(initial_days, aircraft.id, check_type);
            const int due_time = std::max(0, threshold_days - initial) * minutes_per_day;
            if (due_time < horizon) {
                requests.push({due_time, duration, interval, aircraft.id, check_type});
            }
        }
    };

    add_calendar_request('C', data.maintenance_thresholds.C,
                         data.maintenance_durations.C, data.initial_checks.C);
    add_calendar_request('D', data.maintenance_thresholds.D,
                         data.maintenance_durations.D, data.initial_checks.D);

    std::vector<MaintenanceAssignment> result;
    std::vector<CapacityReservation> reservations;
    std::map<std::string, int> aircraft_available_time;
    std::set<std::string> blocked_aircraft;

    while (!requests.empty()) {
        const MaintenanceRequest request = requests.top();
        requests.pop();

        if (request.due_time >= horizon) continue;

        MaintenanceAssignment event{
            request.aircraft_id,
            request.check_type,
            request.due_time,
            request.due_time,
            request.due_time + request.duration,
            "",
            false
        };

        if (blocked_aircraft.find(request.aircraft_id) == blocked_aircraft.end()) {
            std::string airport;
            const int start = find_slot(
                ground_gaps[request.aircraft_id],
                request.due_time,
                request.duration,
                aircraft_available_time[request.aircraft_id],
                data.station_capacity.capacities,
                reservations,
                airport);
            if (start >= 0) {
                event.maintenance_start = start;
                event.maintenance_end = start + request.duration;
                event.airport = airport;
                event.scheduled = true;
                aircraft_available_time[request.aircraft_id] = event.maintenance_end;
                reservations.push_back({event.maintenance_start, event.maintenance_end, airport});

                if (request.repeat_interval > 0) {
                    const int next_due = event.maintenance_end + request.repeat_interval;
                    if (next_due < horizon) {
                        requests.push({next_due, request.duration, request.repeat_interval,
                                       request.aircraft_id, request.check_type});
                    }
                }
            } else {
                blocked_aircraft.insert(request.aircraft_id);
            }
        }

        result.push_back(std::move(event));
    }

    std::sort(result.begin(), result.end(),
              [](const MaintenanceAssignment& left, const MaintenanceAssignment& right) {
                  if (left.maintenance_start != right.maintenance_start) {
                      return left.maintenance_start < right.maintenance_start;
                  }
                  if (left.aircraft_id != right.aircraft_id) {
                      return left.aircraft_id < right.aircraft_id;
                  }
                  return left.check_type < right.check_type;
              });
    return result;
}
