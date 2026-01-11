#pragma

#include <ESPAsyncWebServer.h>
#include <string>
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
    static const char *CSS;
    static int WebServerEnabled;
    static int WiFiConfigurationMode;

    static void InitWebServer(bool startInWiFiConfigurationMode);
    static void SendPage_Root(AsyncWebServerRequest *request);

    static std::string GetComponent_StatsTable();
    static std::string GetPage_Main();
    static std::string GetPage_About();
    static std::string GetPage_Debug();

    static void SendJson_Stats(AsyncWebServerRequest *request);
    static void SendJson_AccessPointList(AsyncWebServerRequest *request);
    static void SendJson_WiFiStatus(AsyncWebServerRequest *request);

    static void StartServer();
    static void StopServer();

    static void RenderIcons();

    static void ListDir(const char *dirname, uint8_t depth = 0);
    static void WebListDir(std::ostringstream *stream, const char *dirname, uint8_t depth = 0);

    static String htmlEncode(const String &in);
    static String htmlDecode(const String &in);

private:
    static int ShowTraffic;

    static void Get_Header(std::ostringstream &stream);
    static void Get_Footer(std::ostringstream &stream);
};

#endif