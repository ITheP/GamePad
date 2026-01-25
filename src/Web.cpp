
#include "Web.h"
#include "GamePad.h"
#include "Stats.h"
#include "Battery.h"
#include "Config.h"
#include "Defines.h"
#include "Arduino.h"
#include <esp_http_server.h>
#include <esp_https_server.h>
#include <sstream>
#include <LittleFS.h>
#include <Prefs.h>
#include <iomanip>
#include "Battery.h"
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
#include "Security.h"

#ifdef WEBSERVER

String ContentType_Html = "text/html; charset=utf-8";
String ContentType_JS = "application/javascript; charset=utf-8";
String ContentType_CSS = "text/css; charset=utf-8";

httpd_handle_t WebServerHTTP = nullptr;
httpd_handle_t WebServerHTTPS = nullptr;

std::string HttpsCertPem;
std::string HttpsKeyPem;

int Web::WebServerEnabled = false;
char WebServerIcon = Icon_Web_Disabled;
int Web::WiFiHotspotMode;
std::map<String, String> Web::HTMLReplacements;

extern Stats *AllStats[];
extern int AllStats_Count;

// WiFi Enabled/Disabled sets relevant icons/status to show if we can get to the web server or not
// Web server if enabled is always running

void Web::WiFiEnabled()
{
    WebServerEnabled = true;
    WebServerIcon = Icon_Web_Enabled;
}

void Web::WiFiDisabled()
{
    WebServerEnabled = false;
    WebServerIcon = Icon_Web_Disabled;
}

static void register_handler(const httpd_uri_t &uri)
{
    if (WebServerHTTP)
    {
        esp_err_t res = httpd_register_uri_handler(WebServerHTTP, &uri);
        if (res != ESP_OK)
            Serial.printf("HTTP handler register failed for %s\n", uri.uri);
        else

            Serial.printf("HTTP handler registered for %s\n", uri.uri);
    }
    if (WebServerHTTPS)
    {
        esp_err_t res = httpd_register_uri_handler(WebServerHTTPS, &uri);
        if (res != ESP_OK)
            Serial.printf("HTTPS handler register failed for %s\n", uri.uri);
        else

            Serial.printf("HTTPS handler registered for %s\n", uri.uri);
    }
}

esp_err_t Web::SendPage_Root(httpd_req_t *req)
{
    File file = LittleFS.open("/root.html", FILE_READ);
    if (!file)
    {
#ifdef EXTRA_SERIAL_DEBUG
        Serial_INFO;
        Serial.println("🌐 Failed to open /root.html");
#endif
        httpd_resp_send_404(req);
        return ESP_OK;
    }

#ifdef EXTRA_SERIAL_DEBUG
    Serial_INFO;
    Serial.println("🌐 Sending /root.html");
#endif

    httpd_resp_set_type(req, "text/html; charset=utf-8");

    const size_t bufferSize = 512;
    uint8_t buffer[bufferSize];
    while (file.available())
    {
        size_t bytesRead = file.read(buffer, bufferSize);
        httpd_resp_send_chunk(req, (const char *)buffer, bytesRead);
    }
    file.close();

    httpd_resp_send_chunk(req, nullptr, 0);
    return ESP_OK;
}

esp_err_t Web::SendPage_Main(httpd_req_t *req)
{
    SendPageWithMergeFields("/main.html", HTMLReplacements, req);
    return ESP_OK;
}

esp_err_t Web::SendPage_Hotspot(httpd_req_t *req)
{
    HTMLReplacements["DeviceName"] = DeviceName;
    HTMLReplacements["Profile"] = CurrentProfile->Description.c_str();
    SendPageWithMergeFields("/hotspot.html", HTMLReplacements, req);
    return ESP_OK;
}

esp_err_t Web::SendPage_About(httpd_req_t *req)
{
    httpd_resp_send_404(req);
    return ESP_OK;
}

esp_err_t Web::SendPage_Debug(httpd_req_t *req)
{
    std::ostringstream html;
    html << "<h1>Debug</h1>";
    html << "Device has rebooted " << Prefs::BootCount << " times since last power on<hr/>";

    std::vector<String> crashLogs;
    Debug::GetCrashLogPaths(crashLogs, true);

    if (crashLogs.empty())
    {
        html << "No crash logs exist";
    }
    else
    {
        html << "<h2>Crash Logs</h2>";

        for (size_t i = 0; i < crashLogs.size(); i++)
        {
            float opacity = 1.0f - (0.07f * static_cast<float>(i));
            if (opacity < 0.0f)
                opacity = 0.0f;

            html << "<div style='opacity:" << std::fixed << std::setprecision(2) << opacity << "'>";
            html << "<strong>" << crashLogs[i].c_str() << "</strong><br/><pre><code>";

            File file = LittleFS.open(crashLogs[i].c_str(), FILE_READ);
            if (file)
            {
                const size_t bufferSize = 512;
                uint8_t buffer[bufferSize];
                while (file.available())
                {
                    size_t bytesRead = file.read(buffer, bufferSize);
                    html.write(reinterpret_cast<const char *>(buffer), bytesRead);
                }
                file.close();
            }
            else
            {
                html << "Failed to open " << crashLogs[i].c_str();
            }

            html << "</code></pre></div><hr/>";
        }
    }

    html << "<hr><h2>Web Files</h2><div style='list-style-type: none'>";
    WebListDir(&html, "/");
    html << "</div><hr><h2>Statistics</h2>";
    Prefs::WebDebug(&html);

    std::string response = html.str();
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
}

esp_err_t Web::SendComponent_StatsTable(httpd_req_t *req)
{
    std::ostringstream table;
    table << "<table border='1'><tr><th rowspan='2'>Name</th><th colspan='4'>Current</th>"
          << "<th colspan='2'>Session</th><th colspan='2'>Ever</th></tr><tr>"
          << "<th>Second Count</th><th>Total Count</th><th>Max Per Second</th>"
          << "<th>Max Per Second Over Last Minute</th><th>Total Count</th><th>Max Per Second</th>"
          << "<th>Total Count</th><th>Max per Second</th></tr>";

    for (int i = 0; i < AllStats_Count; i++)
    {
        table << "<tr><td>" << AllStats[i]->Description << "</td>"
              << "<td>" << AllStats[i]->Current_SecondCount << "</td>"
              << "<td>" << AllStats[i]->Current_TotalCount << "</td>"
              << "<td>" << AllStats[i]->Current_MaxPerSecond << "</td>"
              << "<td>" << AllStats[i]->Current_MaxPerSecondOverLastMinute << "</td>"
              << "<td>" << AllStats[i]->Session_TotalCount << "</td>"
              << "<td>" << AllStats[i]->Session_MaxPerSecond << "</td>"
              << "<td>" << AllStats[i]->Ever_TotalCount << "</td>"
              << "<td>" << AllStats[i]->Ever_MaxPerSecond << "</td></tr>";
    }
    table << "</table>";

    std::string response = table.str();
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
}

esp_err_t Web::Send_DeviceInfo(httpd_req_t *req)
{
    httpd_resp_send_404(req);
    return ESP_OK;
}

esp_err_t Web::Send_BatteryInfo(httpd_req_t *req)
{
    char json[128];
    snprintf(json, sizeof(json),
             "{\"BatteryLevel\":%d, \"BatteryVoltage\":%d}",
             Battery::GetLevel(),
             Battery::Voltage);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    return ESP_OK;
}

esp_err_t Web::Send_Stats(httpd_req_t *req)
{
    std::ostringstream json;
    json << "{\"stats\": [";

    for (int i = 0; i < AllStats_Count; i++)
    {
        if (i > 0)
            json << ",";
        json << "{\"Name\": \"" << AllStats[i]->Description << "\","
             << "\"Current_SecondCount\": " << AllStats[i]->Current_SecondCount << ","
             << "\"Current_TotalCount\": " << AllStats[i]->Current_TotalCount << ","
             << "\"Current_MaxPerSecond\": " << AllStats[i]->Current_MaxPerSecond << ","
             << "\"Current_MaxPerSecondOverLastMinute\": " << AllStats[i]->Current_MaxPerSecondOverLastMinute << ","
             << "\"Session_TotalCount\": " << AllStats[i]->Ever_TotalCount << ","
             << "\"Session_MaxPerSecond\": " << AllStats[i]->Ever_MaxPerSecond << ","
             << "\"Ever_TotalCount\": " << AllStats[i]->Ever_TotalCount << ","
             << "\"Ever_MaxPerSecond\": " << AllStats[i]->Ever_MaxPerSecond << "}";
    }
    json << "]}";

    std::string response = json.str();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
}

esp_err_t Web::Send_AccessPointList(httpd_req_t *req)
{
    std::ostringstream json;
    json << "{\"accessPoints\": [";

    auto aps = Network::AccessPointList;
    int count = 0;

    for (auto &entry : aps)
    {
        AccessPoint *ap = entry.second;
        if (!ap || ap->ssid.length() == 0)
            continue;

        count++;
        if (count > 1)
            json << ",";
        json << "{\"SSID\": \"" << ap->ssid.c_str() << "\",\"RSSI\": " << ap->rssi << "}";
    }
    json << "], \"count\": " << count << "}";

    std::string response = json.str();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
}

esp_err_t Web::Send_WiFiStatus(httpd_req_t *req)
{
    char json[128];
    snprintf(json, sizeof(json),
             "{\"Status\":\"%s\", \"RSSI\":%d}",
             Network::WiFiStatus,
             Network::WiFiStrength);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    return ESP_OK;
}

esp_err_t Web::Send_WiFiTestStatus(httpd_req_t *req)
{
    Network::WiFiTestResult wifiTestResult = Network::CheckTestResults();
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
            resultText = "Test timed out - may be due to an incorrect password";
            break;
        case Network::TEST_FAILED:
            resultText = "Couldn't connect";
            break;
        default:
            resultText = "WiFi is in an unknown state";
        }
    }

    char json[256];
    snprintf(json, sizeof(json), "{\"Status\":\"%s\"}", resultText.c_str());

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    return ESP_OK;
}

esp_err_t Web::Send_HotspotInfo(httpd_req_t *req)
{
    char json[256];

    if (CurrentProfile != nullptr)
    {
        snprintf(json, sizeof(json),
                 "{\"WiFiSSID\":\"%s\",\"WiFiPassword\":\"%s\"}",
                 CurrentProfile->WiFi_Name.c_str(),
                 CurrentProfile->WiFi_Password.c_str());
    }
    else
    {
        snprintf(json, sizeof(json), "{\"WiFiSSID\":\"\",\"WiFiPassword\":\"\"}");
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    return ESP_OK;
}

esp_err_t Web::POST_UpdateWiFiDetails(httpd_req_t *req)
{
    char buf[512];
    int ret = httpd_req_recv(req, buf, sizeof(buf));
    if (ret <= 0)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    String data(buf);
    String ssid, password;

    int ssid_start = data.indexOf("ssid=");
    if (ssid_start >= 0)
    {
        int ssid_end = data.indexOf("&", ssid_start);
        if (ssid_end < 0)
            ssid_end = data.length();
        ssid = data.substring(ssid_start + 5, ssid_end);
    }

    int pass_start = data.indexOf("password=");
    if (pass_start >= 0)
    {
        int pass_end = data.indexOf("&", pass_start);
        if (pass_end < 0)
            pass_end = data.length();
        password = data.substring(pass_start + 9, pass_end);
    }

    if (ssid.length() == 0)
    {
        httpd_resp_send(req, "{\"error\":\"SSID not set\"}", 28);
        return ESP_OK;
    }

    if (ssid.length() > 32)
    {
        httpd_resp_send(req, "{\"error\":\"ssid longer than max allowed (32 chars)\"}", 56);
        return ESP_OK;
    }

    if (password.length() > 63)
    {
        httpd_resp_send(req, "{\"error\":\"password longer than max allowed (63 chars)\"}", 59);
        return ESP_OK;
    }

    if (ssid != CurrentProfile->WiFi_Name)
        Serial.println("🌐 WiFi SSID changed via web interface");

    if (password != CurrentProfile->WiFi_Password)
        Serial.println("🌐 WiFi Password changed via web interface");

    CurrentProfile->WiFi_Name = ssid;
    CurrentProfile->WiFi_Password = password;
    CurrentProfile->Save();

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\"}", 16);
    return ESP_OK;
}

static esp_err_t default_handler(httpd_req_t *req)
{
    String path = req->uri;

    Serial.printf("default_handler: %s\n", path.c_str());

#ifdef EXTRA_SERIAL_DEBUG
    unsigned long start = millis();
    Serial.print("🌐 📩 ");
    Serial.println("WebRequest: " + path);
#endif

    String contentType;

    if (LittleFS.exists(path))
    {
#ifdef EXTRA_SERIAL_DEBUG
        Serial.printf("📄 ✅ File found: %s\n", path.c_str());
#endif
        if (path.endsWith(".html"))
            contentType = "text/html; charset=utf-8";
        else if (path.endsWith(".js"))
            contentType = "application/javascript; charset=utf-8";
        else if (path.endsWith(".css"))
            contentType = "text/css; charset=utf-8";
        else if (path.endsWith(".png"))
            contentType = "image/png";
        else if (path.endsWith(".jpg"))
            contentType = "image/jpeg";
        else if (path.endsWith(".ico"))
            contentType = "image/x-icon";
        else
            contentType = "text/plain";

        httpd_resp_set_type(req, contentType.c_str());
        httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=86400");

        File file = LittleFS.open(path, FILE_READ);
        const size_t bufferSize = 512;
        uint8_t buffer[bufferSize];
        while (file.available())
        {
            size_t bytesRead = file.read(buffer, bufferSize);
            httpd_resp_send_chunk(req, (const char *)buffer, bytesRead);
        }
        file.close();
        httpd_resp_send_chunk(req, nullptr, 0);

#ifdef EXTRA_SERIAL_DEBUG
        unsigned long end = millis();
        Serial.print("🌐  📨 Request took : " + String(end - start) + "ms");
        if (!contentType.isEmpty())
            Serial.print(" - Content-Type: " + contentType);
        Serial.println();
#endif

        return ESP_OK;
    }

#ifdef EXTRA_SERIAL_DEBUG
    Serial.printf("📄 ❌ File NOT found: %s\n", path.c_str());
#endif
    httpd_resp_send_404(req);
    return ESP_OK;
}

static esp_err_t not_found_handler(httpd_req_t *req, httpd_err_code_t err)
{
    const char *method = "UNKNOWN";
    if (req->method == HTTP_GET) method = "GET";
    else if (req->method == HTTP_POST) method = "POST";
    else if (req->method == HTTP_PUT) method = "PUT";
    else if (req->method == HTTP_DELETE) method = "DELETE";
    else if (req->method == HTTP_HEAD) method = "HEAD";

    Serial.printf("HTTPD 404: %s %s (err=%d)\n", method, req->uri, err);
    return ESP_FAIL;
}

void Web::InitHTMLMergeFields()
{
    HTMLReplacements.clear();
    HTMLReplacements["Profile"] = CurrentProfile->Description.c_str();
    HTMLReplacements["DeviceName"] = DeviceName;
    HTMLReplacements["SerialNumber"] = SerialNumber;
    HTMLReplacements["BuildVersion"] = getBuildVersion();
    HTMLReplacements["ControllerType"] = ControllerType;
    HTMLReplacements["ModelNumber"] = ModelNumber;
    HTMLReplacements["FirmwareRevision"] = FirmwareRevision;
    HTMLReplacements["HardwareRevision"] = HardwareRevision;
    HTMLReplacements["SoftwareRevision"] = SoftwareRevision;
}

// Normal web server uses HTTP
void Web::InitWebServer()
{
#ifdef EXTRA_SERIAL_DEBUG
    Serial_INFO;
    Serial.println("🌐 ⚙️ Initialising Web Server configuration...");
#endif

    WiFiHotspotMode = false;
    InitHTMLMergeFields();

    // Recommended open sockets and url handlers
    // HTTP connections: 3–5
    // HTTPS connections: 2–3
    // Total URI handlers: 20–40
    // That's in total - so probably 4 connections between them and 30 URI handlers total
    // Heap free after startup: 120 KB+ recommended

#ifdef EXTRA_SERIAL_DEBUG_PLUS
    Serial.println("🌐 ✅ Pre-HTTP server heap: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("🌐 ✅ Pre-HTTP server max alloc: " + String(ESP.getMaxAllocHeap()) + " bytes");
#endif

    // HTTP Web Server
    httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
    http_config.server_port = 80;
    http_config.max_open_sockets = 4;
    http_config.max_uri_handlers = 16;
    http_config.stack_size = 8192;
    http_config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&WebServerHTTP, &http_config) != ESP_OK) {
        Serial.println("🌐 ❌ Failed to start HTTP server");
        WebServerHTTP = nullptr;
    } else {
        Serial.println("🌐 ✅ HTTP server started on port 80");
    }

// HTTPS Web Server

// #ifdef EXTRA_SERIAL_DEBUG_PLUS
//     Serial.println("🌐 ✅ Pre-HTTPS server heap: " + String(ESP.getFreeHeap()) + " bytes");
//     Serial.println("🌐 ✅ Pre-HTTPS server max alloc: " + String(ESP.getMaxAllocHeap()) + " bytes");
// #endif

    // if (!HttpsCertPem.empty() && !HttpsKeyPem.empty())
    // {
    //     #ifdef EXTRA_SERIAL_DEBUG_PLUS
    //     Serial.printf("Cert PEM length: %d, Key PEM length: %d\n", HttpsCertPem.length(), HttpsKeyPem.length());
    //     Serial.printf("Cert starts with: %.40s\n", HttpsCertPem.c_str());
    //     Serial.printf("Key starts with: %.40s\n", HttpsKeyPem.c_str());
    //     #endif

    //     httpd_ssl_config_t https_conf = HTTPD_SSL_CONFIG_DEFAULT();
    //     https_conf.httpd.server_port = 443;
    //     https_conf.httpd.max_open_sockets = 4;
    //     https_conf.httpd.max_uri_handlers = 16;
    //     https_conf.httpd.stack_size = 10240;
    //     https_conf.httpd.uri_match_fn = httpd_uri_match_wildcard;
        
    //     https_conf.cacert_pem = (const uint8_t *)HttpsCertPem.c_str();
    //     https_conf.cacert_len = HttpsCertPem.length() + 1;
    //     https_conf.prvtkey_pem = (const uint8_t *)HttpsKeyPem.c_str();
    //     https_conf.prvtkey_len = HttpsKeyPem.length() + 1;

    //     if (httpd_ssl_start(&WebServerHTTPS, &https_conf) != ESP_OK)
    //     {
    //         Serial.println("🌐 ❌ Failed to start HTTPS server");
    //         WebServerHTTPS = nullptr;
    //     }
    //     else
    //     {
    //         Serial.println("🌐 ✅ HTTPS server started on port 443");
    //         httpd_register_err_handler(WebServerHTTPS, HTTPD_404_NOT_FOUND, not_found_handler);
    //     }
    // }

    static const httpd_uri_t uri_root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = [](httpd_req_t *req)
        { return Web::SendPage_Root(req); },
    };

    static const httpd_uri_t uri_main = {
        .uri = "/main.html",
        .method = HTTP_GET,
        .handler = [](httpd_req_t *req)
        { return Web::SendPage_Main(req); },
    };

    static const httpd_uri_t uri_debug = {
        .uri = "/debug",
        .method = HTTP_GET,
        .handler = [](httpd_req_t *req)
        { return Web::SendPage_Debug(req); },
    };

    static const httpd_uri_t uri_stats_table = {
        .uri = "/component/stats",
        .method = HTTP_GET,
        .handler = [](httpd_req_t *req)
        { return Web::SendComponent_StatsTable(req); },
    };

    static const httpd_uri_t uri_stats_json = {
        .uri = "/json/stats",
        .method = HTTP_GET,
        .handler = [](httpd_req_t *req)
        { return Web::Send_Stats(req); },
    };

    static const httpd_uri_t uri_wifi_status = {
        .uri = "/json/wifi_status",
        .method = HTTP_GET,
        .handler = [](httpd_req_t *req)
        { return Web::Send_WiFiStatus(req); },
    };

    static const httpd_uri_t uri_battery_info = {
        .uri = "/json/battery_info",
        .method = HTTP_GET,
        .handler = [](httpd_req_t *req)
        { return Web::Send_BatteryInfo(req); },
    };

    register_handler(uri_root);
    register_handler(uri_main);
    register_handler(uri_debug);
    register_handler(uri_stats_table);
    register_handler(uri_stats_json);
    register_handler(uri_wifi_status);
    register_handler(uri_battery_info);

    static const httpd_uri_t uri_default = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = default_handler,
    };
    register_handler(uri_default);

#ifdef EXTRA_SERIAL_DEBUG
    Serial.println("✅ Main mode routes registered");
#endif
}

// Hotspot web server uses HTTPS
void Web::InitWebServer_Hotspot()
{
#ifdef EXTRA_SERIAL_DEBUG
    Serial_INFO;
    Serial.println("🌐 ⚙️ Initialising Web Server configuration - Config Mode...");
#endif

    WiFiHotspotMode = true;
    InitHTMLMergeFields();

    if (!Security::LoadOrGenerateHTTPSCertificates(HttpsCertPem, HttpsKeyPem))
    {
        Serial.println("🌐 ⚠️  No HTTPS certificates available");
    }

    // Start HTTPS first to allocate its resources before HTTP
    if (!HttpsCertPem.empty() && !HttpsKeyPem.empty())
    {
        httpd_ssl_config_t https_conf = HTTPD_SSL_CONFIG_DEFAULT();
        https_conf.httpd.server_port = 443;
        https_conf.httpd.max_open_sockets = 1;
        https_conf.httpd.max_uri_handlers = 10;
        https_conf.httpd.stack_size = 10240;
        https_conf.httpd.uri_match_fn = httpd_uri_match_wildcard;
        https_conf.cacert_pem = (const uint8_t *)HttpsCertPem.c_str();
        https_conf.cacert_len = HttpsCertPem.length() + 1;
        https_conf.prvtkey_pem = (const uint8_t *)HttpsKeyPem.c_str();
        https_conf.prvtkey_len = HttpsKeyPem.length() + 1;

        if (httpd_ssl_start(&WebServerHTTPS, &https_conf) != ESP_OK)
        {
            Serial.println("🌐 ❌ Failed to start HTTPS server");
            WebServerHTTPS = nullptr;
        }
        else
        {
            Serial.println("🌐 ✅ HTTPS server started on port 443");
            httpd_register_err_handler(WebServerHTTPS, HTTPD_404_NOT_FOUND, not_found_handler);
        }
    }

    // httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
    // http_config.server_port = 80;
    // http_config.max_open_sockets = 1;
    // http_config.max_uri_handlers = 10;

    // if (httpd_start(&WebServerHTTP, &http_config) != ESP_OK)
    // {
    //     Serial.println("❌ Failed to start HTTP server");
    //     WebServerHTTP = nullptr;
    // }
    // else
    // {
    //     Serial.println("✅ HTTP server started on port 80");
    // }

    static const httpd_uri_t uri_hotspot = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = [](httpd_req_t *req)
        { return Web::SendPage_Hotspot(req); },
    };

    static const httpd_uri_t uri_hotspot_info = {
        .uri = "/json/hotspot_info",
        .method = HTTP_GET,
        .handler = [](httpd_req_t *req)
        { return Web::Send_HotspotInfo(req); },
    };

    static const httpd_uri_t uri_ap_list = {
        .uri = "/json/access_points",
        .method = HTTP_GET,
        .handler = [](httpd_req_t *req)
        { return Web::Send_AccessPointList(req); },
    };

    static const httpd_uri_t uri_wifi_test = {
        .uri = "/json/wifi_status",
        .method = HTTP_GET,
        .handler = [](httpd_req_t *req)
        { return Web::Send_WiFiTestStatus(req); },
    };

    static const httpd_uri_t uri_update_wifi = {
        .uri = "/api/UpdateWifiDetails",
        .method = HTTP_POST,
        .handler = [](httpd_req_t *req)
        { return Web::POST_UpdateWiFiDetails(req); },
    };

    register_handler(uri_hotspot);
    register_handler(uri_hotspot_info);
    register_handler(uri_ap_list);
    register_handler(uri_wifi_test);
    register_handler(uri_update_wifi);

    static const httpd_uri_t uri_default = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = default_handler,
    };
    register_handler(uri_default);

#ifdef EXTRA_SERIAL_DEBUG
    Serial.println("🌐 ✅ Hotspot mode routes registered");
#endif
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

void Web::SendPageWithMergeFields(const char *path, const std::map<String, String> &replacements, httpd_req_t *req)
{

    File file = LittleFS.open(path, FILE_READ);
    if (!file)
    {
#ifdef EXTRA_SERIAL_DEBUG
        Serial.println("🌐 🚫 Merged Page: File not found - " + String(path));
#endif
        httpd_resp_send_404(req);
        return;
    }

    size_t fileSize = file.size();
    String fileContent;
    fileContent.reserve(fileSize);

    const size_t bufferSize = 512;
    uint8_t buffer[bufferSize];
    while (file.available())
    {
        size_t bytesRead = file.read(buffer, bufferSize);
        fileContent += String(reinterpret_cast<const char *>(buffer), bytesRead);
    }
    file.close();

    std::ostringstream output;

    for (size_t i = 0; i < fileContent.length(); ++i)
    {
        if (fileContent[i] == '{')
        {
            size_t endBrace = fileContent.indexOf('}', i);
            if (endBrace != (size_t)-1 && endBrace - i < 256)
            {
                String tokenName = fileContent.substring(i + 1, endBrace);
                auto it = replacements.find(tokenName);
                if (it != replacements.end())
                {
                    output << it->second.c_str();
                    i = endBrace;
                    continue;
                }
            }
        }
        output << fileContent[i];
    }

    std::string response = output.str();
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, response.c_str(), response.length());

#ifdef EXTRA_SERIAL_DEBUG
    Serial.println("✅ Merged Page: Processed " + String(path) + " (" + String(fileSize) + " bytes)");
#endif
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
            Serial.print("  ");

        if (file.isDirectory())
        {
            Serial.printf("📁 %s/\n", file.name());
            String subdir = String(file.name());
            if (!subdir.startsWith("/"))
                subdir = "/" + subdir;
            ListDir(subdir.c_str(), depth + 1);
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
            WebListDir(stream, subdir.c_str(), depth + 1);
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

int Web::ShowTraffic = -1;
char LastWebServerIcon;

void Web::RenderIcons()
{
    if (WebServerIcon != LastWebServerIcon)
    {
        RenderIcon(WebServerIcon, uiWebServer_xPos, uiWebServer_yPos, 16, 16);
        LastWebServerIcon = WebServerIcon;
    }

    const int TrafficDisplayTime = 5;
    if (ShowTraffic > TrafficDisplayTime)
    {
        RenderIcon(Icon_Web_Traffic, uiWebServerStatus_xPos, uiWebServerStatus_yPos + 1, 5, 11);
        ShowTraffic = TrafficDisplayTime;
    }

    if (ShowTraffic > 0)
        ShowTraffic--;
    else
    {
        Display.fillRect(uiWebServerStatus_xPos, uiWebServerStatus_yPos + 1, 5, 11, C_BLACK);
        ShowTraffic = -1;
    }
}

#endif