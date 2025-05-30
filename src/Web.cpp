#include "Web.h"
#include "GamePad.h"
#include "Stats.h"
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
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
                // Requires / in front of the filename to work with SPIFFS
              { request->send(SPIFFS, "/root.html", "text/html"); });
    //   { Web::SendPage_Root(request); });

    server.on("/component/stats", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", GetComponent_StatsTable().c_str()); });

    server.on("/page/main", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", GetPage_Main().c_str()); });

    server.on("/page/about", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", GetPage_About().c_str()); });

    server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", "<html><h1>Test Page</h1><p>This is a test page.</p></html>"); });


    // server.on("/default.css", HTTP_GET, [](AsyncWebServerRequest *request)
    //           { request->send(200, "text/css", Web::CSS); });

    server.on("/json/stats", HTTP_GET, [](AsyncWebServerRequest *request)
              { Web::SendJson_Stats(request); });

    // Serve static files from SPIFFS
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("about.html");

    server.onNotFound([](AsyncWebServerRequest *request)
                      {
        String path = request->url();
        if (SPIFFS.exists(path)) {
            request->send(SPIFFS, path, "text/html");
        } else {
            request->send(404, "text/plain", "File Not Found");
        } });
}

std::string Web::GetComponent_StatsTable()
{
    std::ostringstream table;
    table << "<table border='1'>"
          << "<tr>"
          << "<th rowspan='2'>Name</th>"
          << "<th colspan='4'>Current</th>"
          << "<th colspan='2'>Session</th>"
          << "<th colspan='2'>Ever</th>"
          << "</tr>"
          << "<tr>"
          << "<th>Second Count</th>"
          << "<th>Total Count</th>"
          << "<th>Max Per Second</th>"
          << "<th>Max Per Second Over Last Minute</th>"
          << "<th>Total Count</th>"
          << "<th>Max Per Second</th>"
          << "<th>Total Count</th>"
          << "<th>Max per Second</th>"
          << "</tr>";

    for (int i = 0; i < AllStats_Count; i++)
    {
        table << "<tr>"
              << "<td>" << AllStats[i]->Description << "</td>"
              << "<td>" << AllStats[i]->Current_SecondCount << "</td>"
              << "<td>" << AllStats[i]->Current_TotalCount << "</td>"
              << "<td>" << AllStats[i]->Current_MaxPerSecond << "</td>"
              << "<td>" << AllStats[i]->Current_MaxPerSecondOverLastMinute << "</td>"
              << "<td>" << AllStats[i]->Session_TotalCount << "</td>"
              << "<td>" << AllStats[i]->Session_MaxPerSecond << "</td>"
              << "<td>" << AllStats[i]->Ever_TotalCount << "</td>"
              << "<td>" << AllStats[i]->Ever_MaxPerSecond << "</td>"
              << "</tr>";
    }

    table << "</table>";
    return table.str();
}

void Web::Get_Header(std::ostringstream &stream)
{
    stream << "<header class='site-header'>"
           << "<h1>" << DeviceName << "</h1>"
           << "</header>";
}

void Web::Get_Footer(std::ostringstream &stream)
{
    stream << "<footer class='site-footer'>"
           << "<p><i>(c) 2025 MisterB @ I The P</i></p>"
           << "</footer>";
}

std::string Web::GetPage_Main()
{
    std::ostringstream html;
    html
        << "<h1>GamePad - " << DeviceName << "</h1>"
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
        << "<script src='main.js' />";

           return html.str();
}

// TODO: Read from file
std::string Web::GetPage_About()
{
    std::ostringstream html;
    html
        << "<h1>About</h1>"
        << "<h2>TO DO</h2>";

           return html.str();
}

void Web::SendPage_Root(AsyncWebServerRequest *request)
{
    std::ostringstream html;
    html << "<!DOCTYPE html>"
         << "<html lang='en'>"
         << "<head>"
         << "<title>GamePad</title>"
         << "<link rel='stylesheet' type='text/css' href='default.css' />"
         << "<script src='main.js' />"
         << "</head>"
         << "<body>";

         Web::Get_Header(html);

    html << "<div class='content'>"
         << "<h1>GamePad - " << DeviceName << "</h1>"
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
         << "</div>"
         << "</div>";    // Content
         Web::Get_Footer(html);

    html << "</body>"
         << "</html>";

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
        statObj.AddMember("Name", rapidjson::Value(AllStats[i]->Description, allocator), allocator);
        statObj.AddMember("Current_SecondCount", AllStats[i]->Current_SecondCount, allocator);
        statObj.AddMember("Current_TotalCount", AllStats[i]->Current_TotalCount, allocator);
        statObj.AddMember("Current_MaxPerSecond", AllStats[i]->Current_MaxPerSecond, allocator);
        statObj.AddMember("Current_MaxPerSecondOverLastMinute", AllStats[i]->Current_MaxPerSecondOverLastMinute, allocator);
        statObj.AddMember("Session_TotalCount", AllStats[i]->Session_TotalCount, allocator);
        statObj.AddMember("Session_MaxPerSecond", AllStats[i]->Session_MaxPerSecond, allocator);
        statObj.AddMember("Ever_TotalCount", AllStats[i]->Ever_TotalCount, allocator);
        statObj.AddMember("Ever_MaxPerSecond", AllStats[i]->Ever_MaxPerSecond, allocator);

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