// All icons are 16x16 but the pixel usage within a single icon might be less
// Assumes correct font is selected

// Some repetition here in code but..
// We had problems stuffing all icons into a single icon file, especially with space for future expansion
// Some icons, their image data caused bitmap file's to be in a state that converters said didn't work!


#include <Screen.h>
#include <GamePad.h>
#include "RenderText.h"

void RenderIcon(unsigned char icon, int xPos, int yPos, int clearWidth, int clearHeight)
{
  if ((clearWidth + clearHeight) > 0)
    Display.fillRect(xPos, yPos, clearWidth, clearHeight, C_BLACK);

  RREIcons.drawChar(xPos, yPos, icon);
}

void RenderBatteryIcon(unsigned char icon, int xPos, int yPos, int clearWidth, int clearHeight)
{
  if ((clearWidth + clearHeight) > 0)
    Display.fillRect(xPos, yPos, clearWidth, clearHeight, C_BLACK);

  RREBatteryIcons.drawChar(xPos, yPos, icon);
}

void RenderControllerIcon(unsigned char icon, int xPos, int yPos, int clearWidth, int clearHeight)
{
  if ((clearWidth + clearHeight) > 0)
    Display.fillRect(xPos, yPos, clearWidth, clearHeight, C_BLACK);

  RREControllerIcons.drawChar(xPos, yPos, icon);
}

void RenderIconRuns(IconRun runs[], int count, RREFont &font)
{
  for (int i = 0; i < count; i++)
  {
    IconRun run = runs[i];
    int xPos = run.XPos;
    int yPos = run.YPos;
    unsigned char c = run.StartIcon;

    for (int j = 0; j < run.Count; j++)
    {
      font.drawChar(xPos, yPos, c);
      xPos += 16;
      // Serial.print(c, HEX);
      // Serial.print(" ");
      c++;
    }
    // Serial.println();
  }
}

// // TODO: Specify font reference in IconRuns in a structure that encapsulates as much so we only need 1 function to do this
// void RenderLogoRuns(IconRun runs[], int count)
// {
//   for (int i = 0; i < count; i++)
//   {
//     IconRun run = runs[i];
//     int xPos = run.XPos;
//     int yPos = run.YPos;
//     unsigned char c = run.StartIcon;

//     for (int j = 0; j < run.Count; j++)
//     {
//       RRELogo.drawChar(xPos, yPos, c);
//       xPos += 16;
//       c++;
//     }
//   }
// }