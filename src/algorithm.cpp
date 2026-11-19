//
// Created by Uliana Trubach on 14.11.2026.
//

#include <fstream>
#include <iostream>
#include <map>
#include <algorithm>
#include <iomanip>
#include <set>
#include <sstream>
#include "../include/models/algorithm.h"
#include "../include/data/json_reader.h"
#include "../include/models/json.h"
#include "../include/data/excel_reader.h"
#include "../include/models/csv.h"

struct AircraftState {
    std::string aircraft_id;
    std::string current_location;
    int av_time;
    int hours_since_maintenance;
    bool maintenance = false;
};

int get_penalty_for_flight(const std::string& origin,
                           const std::string& destination,
                           int day,
                           const std::vector<OD_pairs_and_Penalties>& od_list) {
    for (const auto& od : od_list) {
        if (std::to_string(od.origin_city_id) == origin &&
            std::to_string(od.destination_city_id) == destination &&
            od.day_of_week == day) {
            if (!od.penalties.empty()) {
                return od.penalties[0];
            }
            }
    }
    return 0;
}

int get_day(const std::vector<OD_pairs_and_Penalties>& od_list) {
    if (od_list.empty()) return 1;
    return od_list.at(0).day_of_week;
}

void get_data_json(const char* filename) {
    JsonDataset data = read_json_file(filename);
}

void get_data_csv(const std::string& od_filename, const std::string& maint_filename) {
    std::vector<OD_pairs_and_Penalties> od_list;
    Maintenance maintenance_data;
    try {
        csv_to_Od_pairs(od_filename, od_list);
        csv_to_Maintenance(maint_filename, maintenance_data);
        std::cout << "Successfully loaded " << od_list.size() << " OD pairs and maintenance data!\n";
    } catch (const std::exception& e) {
        std::cerr << "Error loading CSV files: " << e.what() << "\n";
    }
}

std::vector<FlightAssignment> algorithm_tap(JsonDataset data,
                                            const std::vector<OD_pairs_and_Penalties>& od_list,
                                            const Maintenance& maintenance) {

    std::vector<FlightAssignment> schedule;

    auto flights = data.flights;
    auto aircrafts= data.aircraft;

    std::sort(flights.begin(), flights.end(), [](const Flights& a, const Flights& b) {
        return a.start_number < b.start_number;
    });

    std::vector<AircraftState> fleet_states;
    for (const auto& ac: aircrafts) {
        fleet_states.push_back({ac.id, ac.aircraft_init_pos, 0, 0});
    }

    for (const auto& flight: flights) {
        AircraftState* best_aircraft = nullptr;
        int min_cost = 1e9;

        for (auto& ac: fleet_states) {
            if (ac.maintenance) continue;
            bool location = (ac.current_location == flight.init_pos);
            bool time = ac.av_time <= flight.start_number-30;
            if (location && time) {
                int base_cost =  10;
                int day = (!od_list.empty()) ? od_list.at(0).day_of_week : 1;
                int penalty = get_penalty_for_flight(flight.init_pos, flight.dest_pos, day, od_list);
                int total_cost = base_cost + penalty;
                if (total_cost < min_cost) {
                    min_cost = total_cost;
                    best_aircraft = &ac;
                }
            }
        }
        if (best_aircraft != nullptr) {
            FlightAssignment assignment;
            assignment.aircraft_id = best_aircraft->aircraft_id;
            assignment.flight_id = flight.flight_id;
            assignment.flight_start = flight.start_number;
            assignment.flight_end = flight.end_number;
            assignment.day_of_week = get_day(od_list);
            assignment.assigned_airport = flight.init_pos;

            schedule.push_back( assignment );

            best_aircraft->current_location = flight.dest_pos;
            best_aircraft->av_time = flight.end_number;
        }
    }
    return schedule;
}

namespace {
std::string xml_escape(const std::string& value) {
    std::string escaped;
    for (char c : value) {
        switch (c) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&apos;"; break;
            default: escaped += c;
        }
    }
    return escaped;
}

std::string mermaid_escape(const std::string& value) {
    std::string escaped;
    for (char c : value) {
        if (c == ':' || c == ',' || c == ';') escaped += ' ';
        else escaped += c;
    }
    return escaped;
}

std::string javascript_string(const std::string& value) {
    std::ostringstream escaped;
    escaped << '"';
    for (unsigned char c : value) {
        switch (c) {
            case '"': escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            case '<': escaped << "\\u003c"; break;
            case '>': escaped << "\\u003e"; break;
            case '&': escaped << "\\u0026"; break;
            default:
                if (c < 0x20) {
                    escaped << "\\u00" << std::hex << std::setw(2) << std::setfill('0')
                            << static_cast<int>(c) << std::dec;
                } else {
                    escaped << static_cast<char>(c);
                }
        }
    }
    escaped << '"';
    return escaped.str();
}
}

void print_gantt_ascii(const std::vector<FlightAssignment>& schedule, std::ostream& output) {
    std::map<std::string, std::vector<const FlightAssignment*>> by_aircraft;
    for (const auto& assignment : schedule) {
        by_aircraft[assignment.aircraft_id].push_back(&assignment);
    }

    if (schedule.empty()) {
        output << "No flights assigned.\n";
        return;
    }

    int minimum = schedule.front().flight_start;
    int maximum = schedule.front().flight_end;
    for (const auto& assignment : schedule) {
        minimum = std::min(minimum, assignment.flight_start);
        maximum = std::max(maximum, assignment.flight_end);
    }
    if (maximum <= minimum) maximum = minimum + 1;

    constexpr int chart_width = 60;
    output << "Time " << minimum << std::string(chart_width - 4, ' ') << maximum << '\n';
    for (const auto& [aircraft_id, assignments] : by_aircraft) {
        output << "Aircraft " << aircraft_id << '\n';
        for (const auto* assignment : assignments) {
            int begin = static_cast<int>(
                static_cast<double>(assignment->flight_start - minimum) * chart_width / (maximum - minimum));
            int end = static_cast<int>(
                static_cast<double>(assignment->flight_end - minimum) * chart_width / (maximum - minimum));
            begin = std::clamp(begin, 0, chart_width - 1);
            end = std::clamp(end, begin + 1, chart_width);

            output << "  " << std::setw(5) << assignment->flight_start << " |"
                   << std::string(begin, ' ') << std::string(end - begin, '=') << "| "
                   << std::setw(5) << assignment->flight_end << "  Flight "
                   << assignment->flight_id << " (" << assignment->assigned_airport << ")\n";
        }
    }
}

bool write_gantt_mermaid(const std::vector<FlightAssignment>& schedule, const std::string& filename) {
    std::ofstream output(filename);
    if (!output.is_open()) return false;

    output << "gantt\n";
    output << "    title Flight schedule\n";
    output << "    dateFormat X\n";
    output << "    axisFormat %H:%M\n";

    std::map<std::string, std::vector<const FlightAssignment*>> by_aircraft;
    for (const auto& assignment : schedule) {
        by_aircraft[assignment.aircraft_id].push_back(&assignment);
    }
    for (const auto& [aircraft_id, assignments] : by_aircraft) {
        output << "    section Aircraft " << mermaid_escape(aircraft_id) << "\n";
        for (const auto* assignment : assignments) {
            output << "    Flight " << assignment->flight_id
                   << " (" << mermaid_escape(assignment->assigned_airport) << ")"
                   << " :f" << assignment->flight_id << ", "
                   << static_cast<long long>(assignment->flight_start) * 60 << ", "
                   << static_cast<long long>(assignment->flight_end) * 60 << "\n";
        }
    }
    return static_cast<bool>(output);
}

bool write_gantt_svg(const std::vector<FlightAssignment>& schedule, const std::string& filename) {
    std::ofstream output(filename);
    if (!output.is_open()) return false;
    if (schedule.empty()) {
        output << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"800\" height=\"120\">"
               << "<text x=\"20\" y=\"60\">No flights assigned</text></svg>\n";
        return static_cast<bool>(output);
    }

    int minimum = schedule.front().flight_start;
    int maximum = schedule.front().flight_end;
    std::map<std::string, std::vector<const FlightAssignment*>> by_aircraft;
    for (const auto& assignment : schedule) {
        minimum = std::min(minimum, assignment.flight_start);
        maximum = std::max(maximum, assignment.flight_end);
        by_aircraft[assignment.aircraft_id].push_back(&assignment);
    }
    if (maximum <= minimum) maximum = minimum + 1;

    const int left = 180;
    const int plot_width = 900;
    const int row_height = 50;
    const int header_height = 80;
    const int width = left + plot_width + 40;
    const int height = header_height + static_cast<int>(by_aircraft.size()) * row_height + 40;
    const double time_span = static_cast<double>(maximum) - minimum;

    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    output << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width
           << "\" height=\"" << height << "\" viewBox=\"0 0 " << width << " " << height << "\">\n";
    output << "<rect width=\"100%\" height=\"100%\" fill=\"#ffffff\"/>"
           << "<text x=\"" << left << "\" y=\"30\" font-family=\"sans-serif\" font-size=\"20\" font-weight=\"bold\">Flight Schedule Gantt Diagram</text>\n";
    output << "<text x=\"" << left << "\" y=\"52\" font-family=\"sans-serif\" font-size=\"12\">Time: "
           << minimum << " to " << maximum << " (dataset time units)</text>\n";
    for (int tick = 0; tick <= 5; ++tick) {
        const double ratio = tick / 5.0;
        const int x = left + static_cast<int>(ratio * plot_width);
        const double time = minimum + ratio * time_span;
        output << "<line x1=\"" << x << "\" y1=\"" << header_height - 5
               << "\" x2=\"" << x << "\" y2=\"" << height - 25
               << "\" stroke=\"#e5e7eb\" stroke-width=\"1\"/>\n"
               << "<text x=\"" << x << "\" y=\"" << header_height - 12
               << "\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"11\" fill=\"#374151\">"
               << static_cast<long long>(time) << "</text>\n";
    }

    int y = header_height;
    for (const auto& [aircraft_id, assignments] : by_aircraft) {
        output << "<text x=\"20\" y=\"" << y + 25
               << "\" font-family=\"sans-serif\" font-size=\"14\">Aircraft "
               << xml_escape(aircraft_id) << "</text>\n";
        for (const auto* assignment : assignments) {
            int x = left + static_cast<int>(
                (assignment->flight_start - minimum) * static_cast<double>(plot_width) / time_span);
            int bar_width = std::max(2, static_cast<int>(
                (assignment->flight_end - assignment->flight_start) * static_cast<double>(plot_width) / time_span));
            output << "<rect x=\"" << x << "\" y=\"" << y + 8
                   << "\" width=\"" << bar_width << "\" height=\"24\" rx=\"4\" fill=\"#2563eb\">"
                   << "<title>Flight " << assignment->flight_id << ": "
                   << assignment->flight_start << "-" << assignment->flight_end
                   << " from " << xml_escape(assignment->assigned_airport) << "</title></rect>\n";
            output << "<text x=\"" << x + 5 << "\" y=\"" << y + 24
                   << "\" font-family=\"sans-serif\" font-size=\"11\" fill=\"#ffffff\">F"
                   << assignment->flight_id << "</text>\n";
        }
        y += row_height;
    }

    output << "<text x=\"" << left << "\" y=\"" << height - 10
           << "\" font-family=\"sans-serif\" font-size=\"12\">Start and end times use the dataset time units.</text>\n";
    output << "</svg>\n";
    return static_cast<bool>(output);
}

bool write_gantt_html(const std::vector<FlightAssignment>& schedule, const std::string& filename) {
    std::ofstream output(filename);
    if (!output.is_open()) return false;

    std::set<std::string> aircraft;
    int first_day = 0;
    int last_day = 0;
    if (!schedule.empty()) {
        first_day = schedule.front().flight_start / 1440;
        last_day = (schedule.front().flight_end - 1) / 1440;
        for (const auto& assignment : schedule) {
            aircraft.insert(assignment.aircraft_id);
            first_day = std::min(first_day, assignment.flight_start / 1440);
            last_day = std::max(last_day, (assignment.flight_end - 1) / 1440);
        }
    }

    output << R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Flight schedule - Gantt chart</title>
<style>
:root{font-family:Inter,"Segoe UI",Arial,sans-serif;color:#172033;background:#f3f6fb}
*{box-sizing:border-box}
body{margin:0;padding:32px 20px}
main{max-width:1450px;margin:auto}
header,.panel{background:#fff;border:1px solid #e3e9f2;border-radius:14px;box-shadow:0 6px 22px #1720330b}
header{padding:24px 28px;margin-bottom:18px}
h1{font-size:clamp(22px,3vw,30px);margin:0 0 8px}
.subtitle{color:#667085;margin:0}
.stats{display:flex;gap:12px;flex-wrap:wrap;margin-top:18px}
.stat{background:#f5f8fc;border-radius:9px;padding:10px 14px;min-width:135px}
.stat strong{display:block;font-size:19px}.stat span{font-size:12px;color:#667085}
.panel{padding:20px;margin-bottom:18px}
.toolbar{display:flex;align-items:center;gap:10px;flex-wrap:wrap;margin-bottom:14px}
button,select{font:inherit;border:1px solid #d5deea;background:#fff;border-radius:8px;padding:9px 12px;color:#172033}
button{cursor:pointer}button:hover:not(:disabled){background:#f1f5fb}
button:disabled{opacity:.45;cursor:default}
select{min-width:180px}
#day-title{font-size:16px;font-weight:650;margin-left:auto}
.chart-wrap{overflow-x:auto;border:1px solid #e8edf4;border-radius:10px}
svg{display:block;width:100%;min-width:900px;height:auto}
.hint{font-size:12px;color:#667085;margin:10px 2px 0}
.table-wrap{overflow:auto}
table{width:100%;border-collapse:collapse;font-size:14px}
th,td{text-align:left;padding:10px 12px;border-bottom:1px solid #edf0f5;white-space:nowrap}
th{font-size:12px;text-transform:uppercase;letter-spacing:.04em;color:#667085;background:#f8fafc}
tr:last-child td{border-bottom:0}
.empty{padding:26px;text-align:center;color:#667085}
@media(max-width:600px){body{padding:14px 10px}header,.panel{padding:16px}.stat{min-width:110px}#day-title{width:100%;margin-left:0}}
</style>
</head>
<body>
<main>
<header>
  <h1>Flight schedule</h1>
  <p class="subtitle">Daily Gantt view of assigned flights. Choose a day to inspect the timeline.</p>
  <div class="stats">
    <div class="stat"><strong id="flight-count">0</strong><span>assigned flights</span></div>
    <div class="stat"><strong id="aircraft-count">0</strong><span>aircraft in schedule</span></div>
    <div class="stat"><strong id="day-count">0</strong><span>schedule days</span></div>
  </div>
</header>
<section class="panel">
  <div class="toolbar">
    <button id="previous" type="button" aria-label="Previous day">← Previous</button>
    <label for="day-select">Show day</label>
    <select id="day-select"></select>
    <button id="next" type="button" aria-label="Next day">Next →</button>
    <span id="day-title"></span>
  </div>
  <div class="chart-wrap"><svg id="chart" role="img" aria-label="Daily flight Gantt chart"></svg></div>
  <p class="hint">Hover over a flight bar for its route and exact times. Times are shown as hours and minutes from the start of the dataset.</p>
</section>
<section class="panel">
  <h2 id="table-title">Flights for this day</h2>
  <div class="table-wrap">
    <table><thead><tr><th>Aircraft</th><th>Flight</th><th>Route</th><th>Departure</th><th>Arrival</th><th>Duration</th></tr></thead>
    <tbody id="flight-rows"></tbody></table>
    <div id="empty-day" class="empty" hidden>No flights assigned on this day.</div>
  </div>
</section>
</main>
<script>
const flights = [
)HTML";
    for (size_t i = 0; i < schedule.size(); ++i) {
        const auto& assignment = schedule[i];
        output << "  {aircraft:" << javascript_string(assignment.aircraft_id)
               << ", flight:" << assignment.flight_id
               << ", start:" << assignment.flight_start
               << ", end:" << assignment.flight_end
               << ", origin:" << javascript_string(assignment.assigned_airport) << "}"
               << (i + 1 == schedule.size() ? "\n" : ",\n");
    }
    output << R"HTML(];
const aircraftIds = [
)HTML";
    size_t aircraft_index = 0;
    for (const auto& id : aircraft) {
        output << "  " << javascript_string(id)
               << (++aircraft_index == aircraft.size() ? "\n" : ",\n");
    }
    output << R"HTML(];
const firstDay = )HTML" << first_day << R"HTML(;
const lastDay = )HTML" << last_day << R"HTML(;
const colors = ["#2563eb","#0f9f8f","#8b5cf6","#e27a21","#db4777","#4b7f35","#c0392b","#168aad"];
const select = document.getElementById("day-select");
const chart = document.getElementById("chart");
const rows = document.getElementById("flight-rows");
const formatTime = minutes => {
  const hour = Math.floor(minutes / 60);
  const minute = Math.abs(minutes % 60);
  return String(hour).padStart(2,"0") + ":" + String(minute).padStart(2,"0");
};
document.getElementById("flight-count").textContent = flights.length;
document.getElementById("aircraft-count").textContent = aircraftIds.length;
document.getElementById("day-count").textContent = flights.length ? lastDay - firstDay + 1 : 0;
for (let day = firstDay; day <= lastDay; day++) {
  const option = document.createElement("option");
  option.value = day;
  option.textContent = "Day " + (day + 1);
  select.appendChild(option);
}
const svgElement = (name, attributes, text) => {
  const element = document.createElementNS("http://www.w3.org/2000/svg", name);
  for (const [key, value] of Object.entries(attributes)) element.setAttribute(key, value);
  if (text !== undefined) element.textContent = text;
  return element;
};
function renderDay() {
  const day = Number(select.value);
  const dayFlights = flights.filter(f => Math.floor(f.start / 1440) === day)
    .sort((a,b) => a.start - b.start);
  document.getElementById("day-title").textContent = "Day " + (day + 1);
  document.getElementById("table-title").textContent = "Flights for Day " + (day + 1);
  document.getElementById("previous").disabled = day <= firstDay;
  document.getElementById("next").disabled = day >= lastDay;
  rows.replaceChildren();
  document.getElementById("empty-day").hidden = dayFlights.length !== 0;

  const rowHeight = 58, top = 72, left = 150, plotWidth = 1180;
  const maxLocalEnd = dayFlights.reduce((max,f) => Math.max(max, f.end - day * 1440), 1440);
  const timeEnd = Math.max(1440, Math.ceil(maxLocalEnd / 180) * 180);
  const height = top + aircraftIds.length * rowHeight + 20;
  chart.setAttribute("viewBox", `0 0 1380 ${height}`);
  chart.replaceChildren();
  chart.appendChild(svgElement("rect",{x:0,y:0,width:1380,height,fill:"#fff"}));

  for (let tick = 0; tick <= timeEnd; tick += 180) {
    const x = left + tick / timeEnd * plotWidth;
    chart.appendChild(svgElement("line",{x1:x,y1:top-18,x2:x,y2:height-10,stroke:"#e7ecf3","stroke-width":1}));
    chart.appendChild(svgElement("text",{x,y:top-29,"text-anchor":"middle",fill:"#64748b","font-size":12},formatTime(tick)));
  }

  aircraftIds.forEach((aircraft,index) => {
    const y = top + index * rowHeight;
    chart.appendChild(svgElement("text",{x:left-16,y:y+27,"text-anchor":"end",fill:"#344054","font-size":14,"font-weight":600},"Aircraft " + aircraft));
    chart.appendChild(svgElement("line",{x1:left,y1:y+rowHeight-2,x2:left+plotWidth,y2:y+rowHeight-2,stroke:"#edf1f6","stroke-width":1}));
    dayFlights.filter(f => f.aircraft === aircraft).forEach(f => {
      const localStart = f.start - day * 1440, localEnd = f.end - day * 1440;
      const x = left + localStart / timeEnd * plotWidth;
      const width = Math.max(6,(localEnd-localStart) / timeEnd * plotWidth);
      const bar = svgElement("rect",{x,y:y+10,width,height:30,rx:6,fill:colors[index % colors.length],tabindex:0});
      bar.appendChild(svgElement("title",{},`Flight ${f.flight} | ${f.origin} | ${formatTime(localStart)}–${formatTime(localEnd)}`));
      chart.appendChild(bar);
      if (width >= 30) chart.appendChild(svgElement("text",{x:x+7,y:y+30,fill:"#fff","font-size":12,"font-weight":700,"pointer-events":"none"},"F"+f.flight));

      const tr = document.createElement("tr");
      const values = [f.aircraft, "Flight " + f.flight, f.origin, formatTime(localStart), formatTime(localEnd), formatTime(localEnd-localStart)];
      for (const value of values) {
        const td = document.createElement("td");
        td.textContent = value;
        tr.appendChild(td);
      }
      rows.appendChild(tr);
    });
  });
}
document.getElementById("previous").addEventListener("click",() => { select.value = Number(select.value)-1; renderDay(); });
document.getElementById("next").addEventListener("click",() => { select.value = Number(select.value)+1; renderDay(); });
select.addEventListener("change",renderDay);
if (flights.length) select.value = firstDay;
renderDay();
</script>
</body>
</html>
)HTML";
    return static_cast<bool>(output);
}