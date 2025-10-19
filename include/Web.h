#pragma

#include <ESPAsyncWebServer.h>
#include <string>
#include "Stats.h"

extern char *ControllerType;
extern char *ModelNumber;
extern char *FirmwareRevision;
extern char *HardwareRevision;
extern char *SoftwareRevision;

class Web
{
public:
    static const char *CSS;

    static void SetUpRoutes(AsyncWebServer &server);
    static void SendPage_Root(AsyncWebServerRequest *request);

    static std::string GetComponent_StatsTable();
    static std::string GetPage_Main();
    static std::string GetPage_About();
    static std::string GetPage_Debug();

    static void SendJson_Stats(AsyncWebServerRequest *request);

    static void RenderIcons();
    
private:
    static int ShowTraffic;

    static void Get_Header(std::ostringstream &stream);
    static void Get_Footer(std::ostringstream &stream);
};