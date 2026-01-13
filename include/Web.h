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

    static void InitWebServer();
    static void InitWebServer_Hotspot();

    static void SendPage_Root(AsyncWebServerRequest *request);

    static void SendPage_Main(AsyncWebServerRequest *request);
    static void SendPage_About(AsyncWebServerRequest *request);
    static void SendPage_Debug(AsyncWebServerRequest *request);

    static void SendComponent_StatsTable(AsyncWebServerRequest *request);

    static void SendJson_Stats(AsyncWebServerRequest *request);
    static void SendJson_AccessPointList(AsyncWebServerRequest *request);
    static void SendJson_WiFiStatus(AsyncWebServerRequest *request);
    static void SendJson_HotspotInfo(AsyncWebServerRequest *request);

    static void POST_UpdateWiFiDetails(AsyncWebServerRequest *request);

    static void StartServer();
    static void StopServer();

    static void RenderIcons();

    static void ListDir(const char *dirname, uint8_t depth = 0);
    static void WebListDir(std::ostringstream *stream, const char *dirname, uint8_t depth = 0);

    static String htmlEncode(const String &in);
    static String htmlDecode(const String &in);

private:
    static int ShowTraffic;

    static void InitWebServerCustomHandler();
    static void Get_Header(std::ostringstream &stream);
    static void Get_Footer(std::ostringstream &stream);
};

#endif