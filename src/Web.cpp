
#include <ESPAsyncWebServer.h>
#include <sstream>
#include <LittleFS.h>
#include <Prefs.h>
#include <iomanip>
#include "Web.h"
#include "GamePad.h"
#include "Stats.h"
#include "Battery.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "Config.h"
#include "Arduino.h"
#include "IconMappings.h"
#include "Icons.h"
#include "Screen.h"
#include "UI.h"
#include "Utils.h"
#include "Debug.h"

#ifdef WEBSERVER

// Create AsyncWebServer on port 80
AsyncWebServer WebServer(80);
int Web::WebServerEnabled = false;
char WebServerIcon = Icon_Web_Disabled;

extern Stats *AllStats[];
extern int AllStats_Count;

void Web::StartServer()
{
    WebServer.begin();
    WebServerEnabled = true;
    WebServerIcon = Icon_Web_Enabled;
}

void Web::StopServer()
{
    WebServer.end();
    WebServerEnabled = false;
    WebServerIcon = Icon_Web_Disabled;
}

// Number of sub second loops to display traffic for. E.g. 30fps = ~33ms * 5 = 165ms
#define TrafficDisplayTime 5

void Web::SetUpRoutes()
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
    WebServer.onNotFound([](AsyncWebServerRequest *request)
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
                                 request->send(LittleFS, "/root.html", "text/html");
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
                                 if (LittleFS.exists(path))
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

                                     request->send(LittleFS, path, contentType);
                                 }
                                 else
                                 {
                                     request->send(404, "text/plain", "File Not Found");
#ifdef EXTRA_SERIAL_DEBUG
                                     Serial.println("Request for " + request->url() + ": File Not Found (404 returned)");
#endif

                                     success = 0;
                                 }
                             }

#ifdef EXTRA_SERIAL_DEBUG
                             if (1 == success)
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

std::string Web::GetPage_Main()
{
    std::ostringstream html;

    html
        << "<h1>GamePad - " << DeviceName << "</h1>"
        << "Core v" << getBuildVersion() << "<br/>"
        << "<h2>Device Information</h2>"
        << "Controller type: " << ControllerType << "<br/>"
        << "Device name: " << DeviceName << "<br/>"
        << "Model number: " << ModelNumber << "<br/>"
        << "Serial number: " << SerialNumber << "<br/>"
        << "Firmware version: v" << FirmwareRevision
        << ", Hardware version: v" << HardwareRevision
        << ", Software version: v" << SoftwareRevision << "<br/>"
        << "Battery: " << Battery::GetLevel() << "% - " << Battery::Voltage << "v<br/>";

    return html.str();
}

std::string Web::GetPage_Debug()
{
    std::ostringstream html;

    html
        << "<h1>Debug</h1>";

    html << "Device has been booted "
        << Prefs::BootCount << " times<hr/>";

    if (!LittleFS.exists(Debug::CrashFile))
        html << "No crash.log exists";
    else
    {
        File file = LittleFS.open(Debug::CrashFile, FILE_READ);
        if (!file)
        {
            html << Debug::CrashFile << " exists but unable to open it";
        }
        else
        {
            html << Debug::CrashFile << " contents is as follows...<br/><br/>"
                 << "<pre><code>";

            const size_t bufferSize = 512;
            uint8_t buffer[bufferSize];

            while (file.available())
            {
                size_t bytesRead = file.read(buffer, bufferSize);
                html.write(reinterpret_cast<const char *>(buffer), bytesRead);
            }

            file.close();
        }
    }

    html << "<hr>"
         << "<h2>Web Files</h2>"
         << "<div style='list-style-type: none'>";

    WebListDir(&html, "/");

    html << "</div><hr>"
         << "<h2>Statistics</h2>";

    Prefs::WebDebug(&html);

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
char LastWebServerIcon;

void Web::RenderIcons()
{
    if (WebServerIcon != LastWebServerIcon)
    {
        RenderIcon(WebServerIcon, uiWebServer_xPos, uiWebServer_yPos, 16, 16);
        LastWebServerIcon = WebServerIcon;
    }

    // Show traffic when it first happens
    if (ShowTraffic > TrafficDisplayTime)
    {
        RenderIcon(Icon_Web_Traffic, uiWebServerStatus_xPos, uiWebServerStatus_yPos, 5, 11);
        ShowTraffic = TrafficDisplayTime;
    }

    if (ShowTraffic > 0)
        ShowTraffic--; // Little delay
    else
    {
        Display.fillRect(uiWebServerStatus_xPos, uiWebServerStatus_yPos + 1, 5, 11, C_BLACK); // Top pixel doesn't need blanking
        ShowTraffic = -1;
    }
}

void Web::ListDir(const char *dirname, uint8_t depth)
{
    File dir = LittleFS.open(dirname);
    if (!dir || !dir.isDirectory())
    {
        Serial.printf("❌ Failed to open directory: %s\n", dirname);
        return;
    }

    File file = dir.openNextFile();
    while (file)
    {
        for (uint8_t i = 0; i < depth; i++)
            Serial.print("  "); // Indent

        if (file.isDirectory())
        {
            Serial.printf("📁 %s/\n", file.name());

            String subdir = String(file.name());
            if (!subdir.startsWith("/"))
                subdir = "/" + subdir;
            ListDir(subdir.c_str(), depth + 1);

            //ListDir(file.name(), depth + 1); // Recurse into subdirectory
        }
        else
        {
            Serial.printf("📄 %s — %d bytes\n", file.name(), file.size());
        }

        file = dir.openNextFile();
    }
}

void Web::WebListDir(std::ostringstream *stream, const char *dirname, uint8_t depth)
{
    *stream << "<ul>";

    File dir = LittleFS.open(dirname);
    if (!dir || !dir.isDirectory())
    {
        *stream << "❌ Failed to open directory: " << dirname << "</ul>";
        return;
    }

    File file = dir.openNextFile();
    while (file)
    {
        // for (uint8_t i = 0; i < depth; i++)
        //     *stream << ("&nbsp;&nbsp;"); // Indent
        *stream << "<li>";

        if (file.isDirectory())
        {
            *stream << "📁 " << file.name() << "<br/>";

            String subdir = String(file.name());
            if (!subdir.startsWith("/"))
                subdir = "/" + subdir;
            WebListDir(stream, subdir.c_str(), depth + 1);

            //WebListDir(stream, file.name(), depth + 1); // Recurse into subdirectory
        }
        else
        {
            *stream << "📄 " << file.name() << " - ";

            size_t size = file.size();

            if (size < 1024)
                *stream << size << " bytes";
            else
            {
                float kb = static_cast<float>(size) / 1024.0f;
                *stream << std::fixed << std::setprecision(1) << kb << " KB";
            }

            *stream << "<br/>";
        }

        *stream << "</li>";

        file = dir.openNextFile();
    }

    *stream << "</ul>";
}

#endif