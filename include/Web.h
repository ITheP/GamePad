#pragma

#include <ESPAsyncWebServer.h>
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

class Web
{
public:
    using RouteHandler = void(*)(AsyncWebServerRequest*);
    
    static const char *CSS;
    static int WebServerEnabled;
    static int WiFiHotspotMode;
    static std::map<String, RouteHandler> Routes;
    static const char *RootWebPath;
    static std::map<String, String> HTMLReplacements;

    static void InitWebServer();
    static void InitWebServer_Hotspot();

    static void StartServer();
    static void StopServer();

    static void RenderIcons();

    static void ListDir(const char *dirname, uint8_t depth = 0);
    static void WebListDir(std::ostringstream *stream, const char *dirname, uint8_t depth = 0);

    static void SendPageWithMergeFields(const char *path, const std::map<String, String> &replacements, AsyncWebServerRequest *request);

    static String htmlEncode(const String &in);
    static String htmlDecode(const String &in);

private:
    static int ShowTraffic;

    static void SendPage_Root(AsyncWebServerRequest *request);
    static void SendPage_Main(AsyncWebServerRequest *request);
    static void SendPage_Hotspot(AsyncWebServerRequest *request);

    static void SendPage_About(AsyncWebServerRequest *request);
    static void SendPage_Debug(AsyncWebServerRequest *request);

    static void SendComponent_StatsTable(AsyncWebServerRequest *request);

    static void Send_DeviceInfo(AsyncWebServerRequest *request);
    static void Send_BatteryInfo(AsyncWebServerRequest *request);
    static void Send_Stats(AsyncWebServerRequest *request);
    static void Send_AccessPointList(AsyncWebServerRequest *request);
    static void Send_WiFiStatus(AsyncWebServerRequest *request);
    static void Send_WiFiTestStatus(AsyncWebServerRequest *request);
    static void Send_HotspotInfo(AsyncWebServerRequest *request);

    static void POST_UpdateWiFiDetails(AsyncWebServerRequest *request);

    static void InitWebServerCustomHandler();
    static void InitHTMLMergeFields();
    static void Get_Header(std::ostringstream &stream);
    static void Get_Footer(std::ostringstream &stream);

    // static void SendJson(rapidjson::Document &doc, AsyncWebServerRequest *request);
};

#endif