#include "Web.h"
#include "GamePad.h"
#include "Stats.h"
#include <ESPAsyncWebServer.h>
#include <sstream>
#include "Battery.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

extern Stats *AllStats[];
extern int AllStats_Count;

void Web::SetUpRoutes(AsyncWebServer &server)
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { Web::SendPage_Root(request); });

    server.on("/component/stats", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", GetComponent_StatsTable().c_str()); });

    server.on("/default.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/css", Web::CSS); });

    server.on("/json/stats", HTTP_GET, [](AsyncWebServerRequest *request)
              { Web::SendJson_Stats(request); });
}

const char *Web::CSS = R"(
body {
    font-family: Arial, sans-serif;
    background-color: #f4f4f4;
    margin: 0;
    padding: 20px;
}
table {
    width: 100%;
    border-collapse: collapse;
}
th, td {
    padding: 10px;
    text-align: left;
}
th {
    background-color: #4CAF50;
    color: white;
}
tr:nth-child(even) {
    background-color: #f9f9f9;
}
a {
    text-decoration: none;
    color: #4CAF50;
}
a:hover {
    text-decoration: underline;
}
.warning {
    color: red;
    font-weight: bold;
}
)";

std::string Web::GetComponent_StatsTable()
{
    std::ostringstream table;
    table << "<table border='1'><tr><th>Name</th><th>Current Total</th></tr>";

    for (int i = 0; i < AllStats_Count; i++)
    {
        table << "<tr><td>" << AllStats[i]->Description << "</td>"
              << "<td>" << AllStats[i]->Current_TotalCount << "</td></tr>";
    }

    table << "</table>";
    return table.str();
}

void Web::SendPage_Root(AsyncWebServerRequest *request)
{    
    std::ostringstream html;
    html << "<html><head><title>Stats</title></head>"
         << "<link rel='stylesheet' type='text/css' href='/default.css'>"

         << "<script>"
         << "let countdown = 5;"
         << "let refreshInterval;"
         << "function updateTime() {"
         << "  let now = new Date();"
         << "  let timeString = now.getHours().toString().padStart(2, '0') + ':'"
         << "                 + now.getMinutes().toString().padStart(2, '0') + ':'"
         << "                 + now.getSeconds().toString().padStart(2, '0');"
         << "  document.getElementById('currentTime').innerText = timeString;"
         << "}"

         << "function toggleRefresh() {"
         << "  let checkbox = document.getElementById('refreshBox');"
         << "  localStorage.setItem('autoRefresh', checkbox.checked);"
         << "  if (checkbox.checked) {"
         << "    startAutoRefresh();"
         << "  } else {"
         << "    clearInterval(refreshInterval);"
         << "    document.getElementById('countdownDisplay').innerHTML = '';"
         << "  }"
         << "}"

         << "function startAutoRefresh() {"
         << "  countdown = 5;"
         << "  document.getElementById('countdownDisplay').innerHTML = countdown + 's until refresh';"
         << "  refreshInterval = setInterval(() => {"
         << "    countdown--;"
         << "    document.getElementById('countdownDisplay').innerHTML = countdown + 's until refresh';"
         << "    if (countdown <= 0) {"
         << "      fetchStats();"
         << "      countdown = 5;"
         << "    }"
         << "  }, 1000);"
         << "}"

         << "function fetchStats() {"
         << "  fetch('/component/stats')"
         << "    .then(response => {"
         << "      if (!response.ok) { throw new Error('Network response was not ok'); }"
         << "      return response.text();"
         << "    })"
         << "    .then(html => { document.getElementById('statsTable').innerHTML = html; })"
         << "    .catch(() => {"
         << "      document.getElementById('statsTable').innerHTML = '<p class=\"warning\">Sorry, unable to communicate with GamePad to retrieve information. It may be turned off, not connected, lost connection or not have its Wifi and Web Service turned on.</p>';"
         << "    });"
         << "}"

         << "window.onload = function() {"
         << "  let savedState = localStorage.getItem('autoRefresh') !== 'false';"
         << "  document.getElementById('refreshBox').checked = savedState;"
         << "  updateTime();"
         << "  setInterval(updateTime, 1000);"
         << "  if (savedState) {"
         << "    startAutoRefresh();"
         << "  }"
         << "}"
         << "</script>"

         << "<body>"
         << "<h1>GamePad - " << DeviceName << "</h1>"
         << "<p><i>(c) 2025 MisterB @ I The P</i></p>"
         << "<h2>Device Information</h2>"
         << "Controller type: " << ControllerType << "<br/>"
         << "Device name: " << DeviceName << "<br/>"
         << "Model number: " << ModelNumber << "<br/>"
         << "Serial number: " << SerialNumber << "<br/>"
         << "Firmware version: " << FirmwareRevision
         << ", Hardware version: " << HardwareRevision
         << ", Software version: " << SoftwareRevision << "<br/>"
         << "Battery: " << Battery::GetLevel() << "%<br/>"
         << "<h2>Statistics Overview</h2>"
         << "<p>Current Time: <span id='currentTime'></span></p>"
         << "<label><input type='checkbox' id='refreshBox' onchange='toggleRefresh()' checked> Auto-refresh stats every 5s</label>"
         << "<div id='countdownDisplay' style='margin-top:10px; font-size:16px; font-weight:bold;'></div>"
         << "<div id='statsTable'>"
         << Web::GetComponent_StatsTable()
         << "</div></body></html>";

    request->send(200, "text/html", html.str().c_str());
}

void Web::SendJson_Stats(AsyncWebServerRequest *request)
{
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Value statsArray(rapidjson::kArrayType);
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

    for (int i = 0; i < AllStats_Count; i++)
    {
        rapidjson::Value statObj(rapidjson::kObjectType);
        statObj.AddMember("name", rapidjson::Value(AllStats[i]->Description, allocator), allocator);
        statObj.AddMember("current_total", AllStats[i]->Current_TotalCount, allocator);
        statsArray.PushBack(statObj, allocator);
    }

    doc.AddMember("stats", statsArray, allocator);
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    request->send(200, "application/json", buffer.GetString());
}

// void handleTestRequest(AsyncWebServerRequest *request) {
//   if (request->hasParam("num1") && request->hasParam("num2") && request->hasParam("text")) {
//       String num1Str = request->getParam("num1")->value();
//       String num2Str = request->getParam("num2")->value();
//       String text = request->getParam("text")->value();

//       int num1 = num1Str.toInt();
//       int num2 = num2Str.toInt();
//       String result = String(num1 * num2) + " " + text;

//       // Create JSON response
//       rapidjson::Document json;
//       json.SetObject();
//       rapidjson::Document::AllocatorType& allocator = json.GetAllocator();

//       json.AddMember("num1", num1, allocator);
//       json.AddMember("num2", num2, allocator);
//       json.AddMember("text", rapidjson::Value(text.c_str(), allocator), allocator);
//       json.AddMember("result", rapidjson::Value(result.c_str(), allocator), allocator);

//       rapidjson::StringBuffer buffer;
//       rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
//       json.Accept(writer);

//       request->send(200, "application/json", buffer.GetString());
//   } else {
//       request->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
//   }
// }