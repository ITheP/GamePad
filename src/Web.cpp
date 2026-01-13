
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
#include "Defines.h"
#include "Arduino.h"
#include "IconMappings.h"
#include "Icons.h"
#include "Screen.h"
#include "UI.h"
#include "Utils.h"
#include "Debug.h"
#include "Network.h"
#include "MenuFunctions.h"

// Notes:
// Filename references Require / in front of the filename to work with SPIFFS
// Includes code defining icon's of state of things so easy to render visuals in UI

#ifdef WEBSERVER

AsyncWebServer WebServer(80); // Create AsyncWebServer on port 80

int Web::WebServerEnabled = false;
char WebServerIcon = Icon_Web_Disabled;
int Web::WiFiHotspotMode;
std::map<String, Web::RouteHandler> Web::Routes;
const char *Web::RootWebPath;

extern Stats *AllStats[];
extern int AllStats_Count;

// WiFiHotspotMode indicates that the device is in local Hotspot access point mode
// which only happens when we are in config menus.

void Web::StartServer()
{
    WebServer.begin();
    WebServerEnabled = true;
    WebServerIcon = Icon_Web_Enabled;

#ifdef EXTRA_SERIAL_DEBUG
    Serial_INFO;
    Serial.println("🌐 ▶️ Web Server started");
#endif
}

void Web::StopServer()
{
    WebServer.end();
    WebServerEnabled = false;
    WebServerIcon = Icon_Web_Disabled;

#ifdef EXTRA_SERIAL_DEBUG
    Serial_INFO;
    Serial.println("🌐 ⏹️ Web Server stopped");
#endif
}

// Number of sub second loops to display traffic for. E.g. 30fps = ~33ms * 5 = 165ms
#define TrafficDisplayTime 5

void Web::InitWebServer()
{
#ifdef EXTRA_SERIAL_DEBUG
    Serial_INFO;
    Serial.println("🌐 ⚙️ Initialising Web Server configuration...");
#endif

    WiFiHotspotMode = false;

    // Set up the routes for the main web server
    Routes = {
        {"/main", Web::SendPage_Main},
        {"/debug", Web::SendPage_Debug},
        {"/component/stats", Web::SendComponent_StatsTable},
        {"/json/stats", Web::SendJson_Stats}};

    RootWebPath = "/root.html";
    InitWebServerCustomHandler();
}

void Web::InitWebServer_Hotspot()
{
#ifdef EXTRA_SERIAL_DEBUG
    Serial_INFO;
    Serial.println("🌐 ⚙️ Initialising Web Server configuration - Config Mode...");
#endif

    WiFiHotspotMode = true;

    // Set up the routes for the hotspot web server
    Routes = {
        {"/json/hotspot_info", Web::SendJson_HotspotInfo},
        {"/json/access_points", Web::SendJson_AccessPointList},
        {"/json/wifi_status", Web::SendJson_WiFiStatus},
        {"/api/UpdateWifiDetails", Web::POST_UpdateWiFiDetails}};

    // Specific cases setting of WiFi details
    // WebServer.on("/api/UpdateWifiDetails", HTTP_POST, [](AsyncWebServerRequest *request)
    //              {
    //     } });

    RootWebPath = "/hotspot.html";
    InitWebServerCustomHandler();
}

void Web::InitWebServerCustomHandler()
{
    #ifdef EXTRA_SERIAL_DEBUG
    Serial_INFO;
    Serial.println("🌐 ⚙️ Setting up Web Server custom handler...");
    Serial.println("🌐 Registered Routes include");
    Serial.println("  🧭  /");
    for (const auto &route : Routes) {
        Serial.print("  🧭 ");
        Serial.println(route.first.c_str());  // prints the path
    }
#endif

    // We do a lot of the heavy lifting ourselves for sorting the
    // pushing of web pages out - this is to allow us to more easily slap in dynamic content.
    // Can still put in special cases and .on requests if required.
    // Also wanted extra flexibility to debug/print what was going on.
    WebServer.onNotFound([](AsyncWebServerRequest *request)
                         {
                             ShowTraffic = TrafficDisplayTime + 1000; // +1 gives us some leeway to try make sure separate thread doesn't reduce this in the RenderIcon before it displays something

                             String path = request->url();

#ifdef EXTRA_SERIAL_DEBUG
                             unsigned long start = millis();

                             switch (request->method())
                             {
                             case HTTP_GET:
                                 Serial.print("🌐 📩 GET ");
                                 break;
                             case HTTP_POST:
                                 Serial.print("🌐 📩 POST ");
                                 break;
                             case HTTP_PUT:
                                 Serial.print("🌐 📩 PUT ");
                                 break;
                             case HTTP_DELETE:
                                 Serial.print("🌐 📩 DELETE ");
                                 break;
                             default:
                                 Serial.print("🌐 📩 OTHER ");
                                 break;
                             }
                             Serial.println("WebRequest: HTTP " + request->url());
#endif

                             int success = 1;
                             String contentType;

                             // Default page
                             if (path.equals("/"))
                                 request->send(LittleFS, Web::RootWebPath, "text/html");
                             else
                             // Check route mapping
                             {
                                 auto it = Web::Routes.find(path);
                                 if (it != Web::Routes.end())
                                 {
                                     it->second(request);
                                 }
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
                                         Serial.println("🚫 Request for " + request->url() + ": File Not Found (404 returned)");
#endif

                                         success = 0;
                                     }
                                 }
                             }

#ifdef EXTRA_SERIAL_DEBUG
                             if (1 == success)
                             {
                                 unsigned long end = millis();
                                 Serial.print("📨 Request for " + request->url() + " took : " + String(end - start) + "ms");
                                 if (!contentType.isEmpty())
                                     Serial.print(" - Content-Type: " + contentType);

                                 Serial.println();
                             }
#endif
                         });
}

void Web::POST_UpdateWiFiDetails(AsyncWebServerRequest *request)
{
    // if (request->method() == HTTP_GET) {
    //     // handle GET
    // } else if (request->method() == HTTP_POST) {
    //     // handle POST
    // }

    // Handle the form data here
    if (request->hasParam("ssid", true) && request->hasParam("password", true))
    {
        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();

#ifdef EXTRA_SERIAL_DEBUG
        Serial.println("Received WiFi details:");
        Serial.println("SSID: " + ssid);
        Serial.println("Password: " + password);
#endif

        // Reminder - Password isn't actually required for open networks

        // Check for validity
        bool ssidOK = true;
        if (ssid.length() == 0)
        {
            request->send(400, "application/json", "{\"error\":\"SSID not set\"}");
            return;
        }

        if (ssid.length() > 32)
        {
            request->send(400, "application/json", "{\"error\":\"ssid longer than max allowed (32 chars)\"}");
            return;
        }

        for (size_t i = 0; i < ssid.length(); i++)
        {
            char c = ssid[i];
            if (c < 32 || c > 126)
            {
                request->send(400, "application/json", "{\"error\":\"ssid contains invalid characters (must be ascii 32 - 126)\"}");
                return;
            }
        }

        // Strictly speaking, WPA2 passwords can be 8-63 characters long but older standards can be shorter. We allow for 63 or less, or no password.
        if (password.length() > 0)
        {
            if (password.length() > 63)
            {
                request->send(400, "application/json", "{\"error\":\"password longer than max allowed (63 chars)\"}");
                return;
            }

            for (size_t i = 0; i < password.length(); i++)
            {
                char c = password[i];
                if (c < 32 || c > 126)
                {
                    request->send(400, "application/json", "{\"error\":\"password contains invalid characters (must be ascii 32 - 126)\"}");
                    return;
                }
            }
        }

        // Save to your profile object or preferences here

        // bool restartTest = Network::IsWiFiTestInProgress();

        if (ssid != CurrentProfile->WiFi_Name)
            Serial.println("🌐 WiFi SSID changed via web interface");

        if (password != CurrentProfile->WiFi_Password)
            Serial.println("🌐 WiFi Password changed via web interface");

        CurrentProfile->WiFi_Name = ssid;
        CurrentProfile->WiFi_Password = password;

        request->send(200, "application/json", "{\"status\":\"ok\"}");

        // Testing etc. takes place within the Hotspot menu function update loop, so nothing extra to do. All we wanted to do here is update the ssid+password.
    }
    else
    {
        request->send(400, "application/json", "{\"error\":\"missing parameters\"}");
    }
}

void Web::SendJson_AccessPointList(AsyncWebServerRequest *request)
{
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Value accessPointArray(rapidjson::kArrayType);
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

    auto aps = Network::AccessPointList;

    // for (auto &entry : AccessPointList)
    // {
    //     const String &ssid = entry.first;     // the key
    //     AccessPoint *ap = entry.second; // the value

    for (auto &entry : aps)
    {
        // TODO: Make this rhobust incase access points change whilst we are reading them

        // std::map<std::string, std::shared_ptr<AccessPoint>> AccessPointList;
        // AccessPointList[ap->ssid] = ap; // ap is a shared_ptr
        // ...
        // for (auto &kv : AccessPointList) {
        //     auto ap = kv.second; // shared_ptr
        //     if (!ap || ap->ssid.empty()) continue;
        //     // safe to use ap->ssid, ap->rssi
        // }

        AccessPoint *ap = entry.second; // the value

        if (!ap || ap->ssid.length() == 0)
            continue; // skip hidden

        rapidjson::Value apObj(rapidjson::kObjectType);

        // Add SSID as a string
        apObj.AddMember("SSID", rapidjson::Value(ap->ssid.c_str(), allocator), allocator);
        // Add RSSI as an integer
        apObj.AddMember("RSSI", ap->rssi, allocator);

        accessPointArray.PushBack(apObj, allocator);
    }

    doc.AddMember("accessPoints", accessPointArray, allocator);
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    request->send(200, "application/json", buffer.GetString());
}

// TODO: Make this general purpose return single value function
void Web::SendJson_WiFiStatus(AsyncWebServerRequest *request)
{
    // This is similar to processing of test results in menu, but fleshed out a bit for web response (don't have same physical screen space limitations)
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

    Network::WiFiTestResult wifiTestResult = Network::CheckTestResults();
    // Get current test status

    // Display WiFi test results
    String resultText;
    String currentPassword = CurrentProfile->WiFi_Password;
    String currentSSID = CurrentProfile->WiFi_Name;

    if (currentSSID.length() == 0)
        resultText = "No Access Point / SSID / WiFi Name defined";
    else if (currentPassword.length() == 0)
        resultText = "No Password defined";
    else
    {
        switch (wifiTestResult)
        {
        case Network::TEST_NOT_STARTED:
            resultText = "Test has not been started yet";
            break;
        case Network::TEST_SUCCESS:
            resultText = "WiFi connected successfully";
            break;
        case Network::TEST_CONNECTING:
            resultText = "Testing in progress...";
            break;
        case Network::TEST_INVALID_PASSWORD:
            resultText = "Invalid Password";
            break;
        case Network::TEST_SSID_NOT_FOUND:
            resultText = "SSID Not Found";
            break;
        case Network::TEST_TIMEOUT:
            resultText = "Test timed out - his may be due to e.g. an incorrect password";
            break;
        case Network::TEST_FAILED:
            resultText = "Couldn't connect";
            break;
        default:
            resultText = "WiFi is in an unknown state";
        }
    }

    // Add a single member "Status"
    doc.AddMember("Status", rapidjson::Value(htmlEncode(resultText).c_str(), allocator), allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    // Send JSON response
    request->send(200, "application/json", buffer.GetString());
}

void Web::SendJson_HotspotInfo(AsyncWebServerRequest *request)
{
    // This is similar to processing of test results in menu, but fleshed out a bit for web response (don't have same physical screen space limitations)
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

    // Device Information
    doc.AddMember("BuildVersion", rapidjson::Value(getBuildVersion(), allocator), allocator);
    doc.AddMember("ControllerType", rapidjson::Value(ControllerType, allocator), allocator);
    doc.AddMember("ModelNumber", rapidjson::Value(ModelNumber, allocator), allocator);
    doc.AddMember("FirmwareRevision", rapidjson::Value(FirmwareRevision, allocator), allocator);
    doc.AddMember("HardwareRevision", rapidjson::Value(HardwareRevision, allocator), allocator);
    doc.AddMember("SoftwareRevision", rapidjson::Value(SoftwareRevision, allocator), allocator);

    // Profile Information
    if (CurrentProfile != nullptr)
    {
        doc.AddMember("ProfileDescription", rapidjson::Value(CurrentProfile->Description.c_str(), allocator), allocator);
        doc.AddMember("WiFiSSID", rapidjson::Value(CurrentProfile->WiFi_Name.c_str(), allocator), allocator);
        doc.AddMember("WiFiPassword", rapidjson::Value(CurrentProfile->WiFi_Password.c_str(), allocator), allocator);
    }
    else
    {
        doc.AddMember("ProfileDescription", rapidjson::Value("Unknown", allocator), allocator);
        doc.AddMember("WiFiSSID", rapidjson::Value("", allocator), allocator);
        doc.AddMember("WiFiPassword", rapidjson::Value("", allocator), allocator);
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    // Send JSON response
    request->send(200, "application/json", buffer.GetString());
}

String Web::htmlEncode(const String &in)
{
    String out;
    for (size_t i = 0; i < in.length(); i++)
    {
        char c = in[i];
        switch (c)
        {
        case '&':
            out += "&amp;";
            break;
        case '<':
            out += "&lt;";
            break;
        case '>':
            out += "&gt;";
            break;
        case '"':
            out += "&quot;";
            break;
        case '\'':
            out += "&#39;";
            break;
        default:
            out += c;
            break;
        }
    }
    return out;
}

String Web::htmlDecode(const String &in)
{
    String out = in;
    out.replace("&amp;", "&");
    out.replace("&lt;", "<");
    out.replace("&gt;", ">");
    out.replace("&quot;", "\"");
    out.replace("&#39;", "'");
    return out;
}

void Web::SendComponent_StatsTable(AsyncWebServerRequest *request)
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

    request->send(200, "text/html", table.str().c_str());
}

void Web::SendPage_Main(AsyncWebServerRequest *request)
{
    std::ostringstream html;

    html
        << "<h1>GamePad - " << DeviceName << "</h1>"
        << "Core " << getBuildVersion() << "<br/>"
        << "<h2>Device Information</h2>"
        << "Controller type: " << ControllerType << "<br/>"
        << "Model number: " << ModelNumber << "<br/>"
        << "Firmware version: v" << FirmwareRevision
        << ", Hardware version: v" << HardwareRevision
        << ", Software version: v" << SoftwareRevision << "<br/>"
        << "Device name: " << DeviceName << "<br/>"
        << "Serial number: " << SerialNumber << "<br/>"
        << "Battery: " << Battery::GetLevel() << "% - " << Battery::Voltage << "v<br/>";

    request->send(200, "text/html", html.str().c_str());
}

// std::string Web::SaveWiFiSettings(const std::string &ssid, const std::string &password)
// {
//     std::ostringstream html;
//
//     if (WiFiHotspotMode)
//     {
//         // Save WiFi details to current profile
//         MenuFunctions::Current_Profile->WiFi_Name = String(ssid.c_str());
//         CurrentProfile->WiFi_Password = String(password.c_str());
//         Profiles::SaveProfile(CurrentProfile);
//
//         html << "<h1>WiFi Settings Saved</h1>"
//              << "<p>The WiFi settings have been saved to the current profile.</p>"
//              << "<p>Please reboot the device to connect to the new WiFi network.</p>";
//     }
//     else
//     {
//         html << "<h1>Error</h1>"
//              << "<p>The device is not in WiFi Configuration Mode. Cannot save WiFi settings.</p>";
//     }
//
//     return html.str();
// }
//
// std::string Web::GetPage_Profiles()
// {
//     std::ostringstream html;
//
//     html
//         << "<h1>GamePad - " << DeviceName << "</h1>"
//         << "Core v" << getBuildVersion() << "<br/>"
//         << "<h2>Device Information</h2>"
//         << "Controller type: " << ControllerType << "<br/>"
//         << "Device name: " << DeviceName << "<br/>"
//         << "Model number: " << ModelNumber << "<br/>"
//         << "Serial number: " << SerialNumber << "<br/>"
//         << "Firmware version: v" << FirmwareRevision
//         << ", Hardware version: v" << HardwareRevision
//         << ", Software version: v" << SoftwareRevision << "<br/>"
//         << "Battery: " << Battery::GetLevel() << "% - " << Battery::Voltage << "v<br/>";
//
//     return html.str();
// }

void Web::SendPage_Debug(AsyncWebServerRequest *request)
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
            ListDir(subdir.c_str(), depth + 1); // Recurse into subdirectory
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
        *stream << "<li>";

        if (file.isDirectory())
        {
            *stream << "📁 " << file.name() << "<br/>";

            String subdir = String(file.name());
            if (!subdir.startsWith("/"))
                subdir = "/" + subdir;
            WebListDir(stream, subdir.c_str(), depth + 1); // Recurse into subdirectory
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