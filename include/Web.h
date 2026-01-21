#pragma once

#include <Arduino.h>
#include <esp_http_server.h>
#include <esp_https_server.h>
#include <string>
#include <map>
#include "Stats.h"
#include "DeviceConfig.h"

#ifdef WEBSERVER

extern char ControllerType[];
extern char ModelNumber[];
extern char FirmwareRevision[];
extern char HardwareRevision[];
extern char SoftwareRevision[];

// Pure ESP-IDF: esp_http_server for HTTP (port 80), esp_https_server for HTTPS (port 443)
extern httpd_handle_t WebServerHTTP;
extern httpd_handle_t WebServerHTTPS;

class Web
{
public:
    static const char *CSS;
    static int WebServerEnabled;
    static int WiFiHotspotMode;
    static const char *RootWebPath;
    static std::map<String, String> HTMLReplacements;

    static void InitWebServer();
    static void InitWebServer_Hotspot();

    static void WiFiEnabled();
    static void WiFiDisabled();

    static void RenderIcons();

    static void ListDir(const char *dirname, uint8_t depth = 0);
    static void WebListDir(std::ostringstream *stream, const char *dirname, uint8_t depth = 0);

    static void SendPageWithMergeFields(const char *path, const std::map<String, String> &replacements, httpd_req_t *req);

    static String htmlEncode(const String &in);
    static String htmlDecode(const String &in);

private:
    static int ShowTraffic;

    static esp_err_t SendPage_Root(httpd_req_t *req);
    static esp_err_t SendPage_Main(httpd_req_t *req);
    static esp_err_t SendPage_Hotspot(httpd_req_t *req);

    static esp_err_t SendPage_About(httpd_req_t *req);
    static esp_err_t SendPage_Debug(httpd_req_t *req);

    static esp_err_t SendComponent_StatsTable(httpd_req_t *req);

    static esp_err_t Send_DeviceInfo(httpd_req_t *req);
    static esp_err_t Send_BatteryInfo(httpd_req_t *req);
    static esp_err_t Send_Stats(httpd_req_t *req);
    static esp_err_t Send_AccessPointList(httpd_req_t *req);
    static esp_err_t Send_WiFiStatus(httpd_req_t *req);
    static esp_err_t Send_WiFiTestStatus(httpd_req_t *req);
    static esp_err_t Send_HotspotInfo(httpd_req_t *req);

    static esp_err_t POST_UpdateWiFiDetails(httpd_req_t *req);

    static void InitWebServerCustomHandler();
    static void InitHTMLMergeFields();
    static void Get_Header(std::ostringstream &stream);
    static void Get_Footer(std::ostringstream &stream);

    // static void SendJson(rapidjson::Document &doc, AsyncWebServerRequest *request);
};

#endif
