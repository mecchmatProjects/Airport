//
// Created by Uliana Trubach on 19.11.2026.
// code written by Natalia Nikolayets
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
    return write_gantt_html(schedule, filename, {}, {}, 0);
}

bool write_gantt_html(const std::vector<FlightAssignment>& schedule,
                      const std::string& filename,
                      const std::vector<MaintenanceAssignment>& maintenance_schedule,
                      const std::vector<Aircraft>& aircraft_list,
                      int horizon_days) {
    std::ofstream output(filename);
    if (!output.is_open()) return false;

    std::set<std::string> aircraft;
    for (const auto& item : aircraft_list) {
        aircraft.insert(item.id);
    }

    int last_day = std::max(0, horizon_days - 1);
    for (const auto& assignment : schedule) {
        aircraft.insert(assignment.aircraft_id);
        last_day = std::max(last_day, (assignment.flight_end - 1) / 1440);
    }
    for (const auto& maintenance : maintenance_schedule) {
        aircraft.insert(maintenance.aircraft_id);
        const int time = maintenance.scheduled ? maintenance.maintenance_end : maintenance.due_time;
        last_day = std::max(last_day, time / 1440);
    }
    const int first_day = 0;

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
.legend{display:flex;gap:16px;flex-wrap:wrap;font-size:12px;color:#475569;margin:10px 2px 0}
.legend span{display:inline-flex;align-items:center;gap:6px}
.legend i{display:inline-block;width:13px;height:13px;border-radius:3px}
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
    <div class="stat"><strong id="aircraft-count">0</strong><span>aircraft in fleet</span></div>
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
  <div class="legend">
    <span><i style="background:#2563eb"></i>Flight</span>
    <span><i style="background:#f59e0b"></i>Scheduled maintenance</span>
    <span><i style="background:#dc2626"></i>Maintenance conflict</span>
  </div>
  <p class="hint">Maintenance is placed in available ground gaps at stations with capacity. Red markers indicate checks that could not fit before the next flight. Hover over a bar for details.</p>
</section>
<section class="panel">
  <h2 id="table-title">Schedule for this day</h2>
  <div class="table-wrap">
    <table><thead><tr><th>Aircraft</th><th>Activity</th><th>Route / Station</th><th>Start</th><th>End</th><th>Duration / Status</th></tr></thead>
    <tbody id="flight-rows"></tbody></table>
    <div id="empty-day" class="empty" hidden>No flights or maintenance on this day.</div>
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
const maintenance = [
)HTML";
    for (size_t i = 0; i < maintenance_schedule.size(); ++i) {
        const auto& event = maintenance_schedule[i];
        output << "  {aircraft:" << javascript_string(event.aircraft_id)
               << ", check:" << javascript_string(std::string(1, event.check_type))
               << ", due:" << event.due_time
               << ", start:" << event.maintenance_start
               << ", end:" << event.maintenance_end
               << ", airport:" << javascript_string(event.airport)
               << ", scheduled:" << (event.scheduled ? "true" : "false") << "}"
               << (i + 1 == maintenance_schedule.size() ? "\n" : ",\n");
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
const formatServiceHours = minutes => {
  const hours = Math.floor(minutes / 60);
  const remainingMinutes = minutes % 60;
  return hours + "h " + String(remainingMinutes).padStart(2,"0") + "m";
};
document.getElementById("flight-count").textContent = flights.length;
document.getElementById("aircraft-count").textContent = aircraftIds.length;
document.getElementById("day-count").textContent = lastDay - firstDay + 1;
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
  const dayStart = day * 1440, dayEnd = dayStart + 1440;
  const dayFlights = flights.filter(f => Math.floor(f.start / 1440) === day)
    .sort((a,b) => a.start - b.start);
  const dayMaintenance = maintenance.filter(m => m.scheduled
    ? m.start < dayEnd && m.end > dayStart
    : Math.floor(m.due / 1440) === day);
  document.getElementById("day-title").textContent = "Day " + (day + 1);
  document.getElementById("table-title").textContent = "Schedule for Day " + (day + 1);
  document.getElementById("previous").disabled = day <= firstDay;
  document.getElementById("next").disabled = day >= lastDay;
  rows.replaceChildren();
  document.getElementById("empty-day").hidden = dayFlights.length + dayMaintenance.length !== 0;

  const rowHeight = 72, top = 72, left = 190, plotWidth = 1140;
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
    const aircraftFlights = dayFlights.filter(f => f.aircraft === aircraft);
    const serviceMinutes = aircraftFlights.reduce((total,f) => total + f.end - f.start, 0);
    chart.appendChild(svgElement("text",{x:left-16,y:y+23,"text-anchor":"end",fill:"#344054","font-size":14,"font-weight":600},"Aircraft " + aircraft));
    chart.appendChild(svgElement("rect",{x:left-160,y:y+33,width:160,height:19,rx:9,fill:"#ecfdf5",stroke:"#a7f3d0"}));
    chart.appendChild(svgElement("text",{x:left-80,y:y+46,"text-anchor":"middle",fill:"#0f766e","font-size":11,"font-weight":600},"Service hours: " + formatServiceHours(serviceMinutes)));
    chart.appendChild(svgElement("line",{x1:left,y1:y+rowHeight-2,x2:left+plotWidth,y2:y+rowHeight-2,stroke:"#edf1f6","stroke-width":1}));
    dayMaintenance.filter(m => m.aircraft === aircraft).forEach(m => {
      if (m.scheduled) {
        const visibleStart = Math.max(dayStart,m.start), visibleEnd = Math.min(dayEnd,m.end);
        const localStart = visibleStart - dayStart, localEnd = visibleEnd - dayStart;
        const x = left + localStart / timeEnd * plotWidth;
        const width = Math.max(4,(localEnd-localStart) / timeEnd * plotWidth);
        const bar = svgElement("rect",{x,y:y+8,width,height:24,rx:5,fill:"#f59e0b",stroke:"#b45309",tabindex:0});
        bar.appendChild(svgElement("title",{},`Maintenance ${m.check} at ${m.airport} | ${formatTime(localStart)}–${formatTime(localEnd)}`));
        chart.appendChild(bar);
        if (width >= 45) chart.appendChild(svgElement("text",{x:x+width/2,y:y+24,"text-anchor":"middle",fill:"#422006","font-size":10,"font-weight":700,"pointer-events":"none"},m.check+"-check"));
      } else {
        const dueLocal = Math.max(0,Math.min(1440,m.due-dayStart));
        const x = left + dueLocal / timeEnd * plotWidth;
        const marker = svgElement("rect",{x:x-5,y:y+6,width:10,height:28,rx:3,fill:"#dc2626",tabindex:0});
        marker.appendChild(svgElement("title",{},`Maintenance ${m.check} could not be scheduled before the next flight (due ${formatTime(m.due-dayStart)})`));
        chart.appendChild(marker);
        chart.appendChild(svgElement("text",{x,y:y+25,"text-anchor":"middle",fill:"#fff","font-size":10,"font-weight":700,"pointer-events":"none"},"!"));
      }
    });
    aircraftFlights.forEach(f => {
      const localStart = f.start - day * 1440, localEnd = f.end - day * 1440;
      const x = left + localStart / timeEnd * plotWidth;
      const width = Math.max(6,(localEnd-localStart) / timeEnd * plotWidth);
      const serviceTime = formatTime(localStart) + "–" + formatTime(localEnd);
      const bar = svgElement("rect",{x,y:y+8,width,height:24,rx:6,fill:colors[index % colors.length],tabindex:0});
      bar.appendChild(svgElement("title",{},`Flight ${f.flight} | ${f.origin} | Service: ${serviceTime} (${formatServiceHours(localEnd-localStart)})`));
      chart.appendChild(bar);
      if (width >= 30) chart.appendChild(svgElement("text",{x:x+7,y:y+24,fill:"#fff","font-size":12,"font-weight":700,"pointer-events":"none"},"F"+f.flight));
      chart.appendChild(svgElement("text",{x:x+width/2,y:y+49,"text-anchor":"middle",fill:"#475569","font-size":10,"pointer-events":"none"},serviceTime));

      const tr = document.createElement("tr");
      const values = [f.aircraft, "Flight " + f.flight, f.origin, formatTime(localStart), formatTime(localEnd), formatTime(localEnd-localStart)];
      for (const value of values) {
        const td = document.createElement("td");
        td.textContent = value;
        tr.appendChild(td);
      }
      rows.appendChild(tr);
    });
    dayMaintenance.filter(m => m.aircraft === aircraft).forEach(m => {
      const start = m.scheduled ? Math.max(dayStart,m.start) : m.due;
      const end = m.scheduled ? Math.min(dayEnd,m.end) : m.due;
      const values = [
        m.aircraft,
        "Maintenance " + m.check + (m.scheduled ? "" : " (conflict)"),
        m.airport || "No available station",
        formatTime(start-dayStart),
        m.scheduled ? formatTime(end-dayStart) : "Not scheduled",
        m.scheduled ? formatServiceHours(m.end-m.start) : "Overdue"
      ];
      const tr = document.createElement("tr");
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