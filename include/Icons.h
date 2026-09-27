#pragma once

#include "Structs.h"

void RenderIcon(unsigned char icon, int xPos, int yPos, int clearWidth, int clearHeight);
void RenderBatteryIcon(unsigned char icon, int xPos, int yPos, int clearWidth, int clearHeight);
void RenderControllerIcon(unsigned char icon, int xPos, int yPos, int clearWidth, int clearHeight);
void RenderIconRuns(IconRun runs[], int count, RREFont &font);
// void RenderLogoRuns(IconRun runs[], int count);