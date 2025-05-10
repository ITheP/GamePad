#pragma

#include <ESPAsyncWebServer.h>
#include <string>
#include "Stats.h"

extern char ControllerType[];
extern char ModelNumber[];
extern char FirmwareRevision[];
extern char HardwareRevision[];
extern char SoftwareRevision[];

class Web
{
public:
    static const char *CSS;

    static void SetUpRoutes(AsyncWebServer &server);
    //static void HandleRequest(AsyncWebServerRequest *request);
    static void SendPage_Root(AsyncWebServerRequest *request);

    static std::string GetComponent_StatsTable();

    static void SendJson_Stats(AsyncWebServerRequest *request);
};