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
#include "Config.h"
#include "Arduino.h"
#include "IconMappings.h"
#include "Icons.h"
#include "Screen.h"

extern Stats *AllStats[];
extern int AllStats_Count;

// Number of sub second loops to display traffic for. E.g. 30fps = ~33ms * 5 = 165ms
#define TrafficDisplayTime 5

void Web::SetUpRoutes(AsyncWebServer &server)
{
    //     server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    //               {
    // #ifdef EXTRA_SERIAL_DEBUG
    //                   ShowRequestStart(request);
    // #endif
    //                   // Requires / in front of the filename to work with SPIFFS
    //                   request->send(SPIFFS, "/root.html", "text/html");
    // #ifdef EXTRA_SERIAL_DEBUG
    //                   ShowRequestEnd(request);
    // #endif
    //               });

    // Serve static files from SPIFFS
    // server.serveStatic("/", SPIFFS, "/").setDefaultFile("root.html");

    // We do a lot of the heavy lifting ourselves for sorting the
    // pushing of web pages out - this is to allow us to
    // more easily slap in dynamic content. Can still put in special cases and
    // .on requests if required.
    // Also wanted extra flexibility to debug/print what was going on
    server.onNotFound([](AsyncWebServerRequest *request)
                      {
                          ShowTraffic = TrafficDisplayTime + 1000; // +1 gives us some leeway to try make sure separate thread doesn't reduce this in the RenderIcon before it displays something

                          String path = request->url();

#ifdef EXTRA_SERIAL_DEBUG
                          unsigned long start = millis();

                          switch (request->method())
                          {
                          case HTTP_GET:
                              Serial.print("GET ");
                              break;
                          case HTTP_POST:
                              Serial.print("POST ");
                              break;
                          case HTTP_PUT:
                              Serial.print("PUT ");
                              break;
                          case HTTP_DELETE:
                              Serial.print("DELETE ");
                              break;
                          default:
                              Serial.print("OTHER ");
                              break;
                          }
                          Serial.println("WebRequest: HTTP " + request->url());
#endif

                          int success = 1;
                          String contentType;

                          // Default page
                          if (path.equals("/"))
                              // Requires / in front of the filename to work with SPIFFS
                              request->send(SPIFFS, "/root.html", "text/html");
                          // special cases
                          else if (path.equals("/main"))
                              request->send(200, "text/html", GetPage_Main().c_str());
                          else if (path.equals("/debug"))
                              request->send(200, "text/html", GetPage_Debug().c_str());
                          else if (path.equals("/component/stats"))
                              request->send(200, "text/html", GetComponent_StatsTable().c_str());
                          else if (path.equals("/json/stats"))
                              Web::SendJson_Stats(request);
                          else
                          {
                              // if (path.startsWith("/page/"))
                              // path = path.substring(6) + ".html"; // Convert /page/name to /name.html
                              if (SPIFFS.exists(path))
                              {
                                  if (path.endsWith(".html"))
                                      contentType = "text/html";
                                  else if (path.endsWith(".js"))
                                      contentType = "application/javascript";
                                  else if (path.endsWith(".css"))
                                      contentType = "text/css";
                                  else if (path.endsWith(".png"))
                                      contentType = "image/png";
                                  else if (path.endsWith(".jpg"))
                                      contentType = "image/jpeg";
                                  else if (path.endsWith(".ico"))
                                      contentType = "image/x-icon";
                                  else
                                      contentType = "text/plain";
                                  // else if (path.endsWith(".gif")) contentType = "image/gif";

                                  request->send(SPIFFS, path, contentType);
                              }
                              else
                              {
                                  request->send(404, "text/plain", "File Not Found");
                                  Serial.println("Request for " + request->url() + ": File Not Found (404 returned)");

                                  success = 0;
                              }
                          }

#ifdef EXTRA_SERIAL_DEBUG
                          if (success == 1)
                          {
                              unsigned long end = millis();
                              Serial.print("Request for " + request->url() + " took : " + String(end - start) + "ms");
                              if (!contentType.isEmpty())
                                  Serial.print(" - Content-Type: " + contentType);

                              Serial.println();
                          }
#endif
                      });
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

// void Web::Get_Header(std::ostringstream &stream)
// {
//     stream << "<header class='site-header'>"
//            << "<h1>" << DeviceName << "</h1>"
//            << "</header>";
// }

// void Web::Get_Footer(std::ostringstream &stream)
// {
//     stream << "<footer class='site-footer'>"
//            << "<p><i>(c) 2025 MisterB @ I The P</i></p>"
//            << "</footer>";
// }

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
        << "<p>Last update time: <span id='currentTime'></span></p>"
        << "<label><input type='checkbox' id='refreshBox' onchange='toggleRefresh()' checked> Auto-refresh stats every 5s</label>"
        << "<div id='countdownDisplay' style='margin-top:10px; font-size:16px; font-weight:bold;'></div>"
        << "<div id='statsTable'>"
        << Web::GetComponent_StatsTable()
        << "</div>";

    return html.str();
}

std::string Web::GetPage_Debug()
{
    std::ostringstream html;
    html
        << "<h1>Debug</h1>"
        << "<h2>TO DO</h2>";

    return html.str();
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


int Web::ShowTraffic = -1;

void Web::RenderIcons()
{
    // if (FinalWifiCharacter != LastWifiCharacter)
    // {
         RenderIcon(Icon_Web_Enabled, uiWebServer_xPos, uiWebServer_yPos, 16, 16);
    //     LastWifiCharacter = FinalWifiCharacter;
    // }
    // if (WifiStatusCharacter != LastWifiStatusCharacter)
    // {

    // Show traffic when it first happens
    if (ShowTraffic > TrafficDisplayTime) {
         RenderIcon(Icon_Web_Traffic, uiWebServerStatus_xPos, uiWebServerStatus_yPos, 5, 11);
         ShowTraffic = TrafficDisplayTime;
    }

    if (ShowTraffic > 0)
        ShowTraffic--;  // Little delay
    else
    {
        Display.fillRect(uiWebServerStatus_xPos, uiWebServerStatus_yPos + 1, 5, 11, C_BLACK ); // Top pixel doesn't need blanking
        ShowTraffic = -1;
    }
}