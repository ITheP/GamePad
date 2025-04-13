// All icons are 16x16 but the pixel usage within a single icon might be less
// Assumes correct font is selected

#include <Screen.h>
#include <GamePad.h>

void RenderIcon(unsigned char icon, int xPos, int yPos, int clearWidth, int clearHeight)
{
  if ((clearWidth + clearHeight) > 0)
    Display.fillRect(xPos, yPos, clearWidth, clearHeight, C_BLACK);

  RRE.drawChar(xPos, yPos, icon);
}

// Assumes correct font is selected
void RenderIconRuns(IconRun runs[], int count)
{
  for (int i = 0; i < count; i++)
  {
    IconRun run = runs[i];
    int xPos = run.XPos;
    int yPos = run.YPos;
    unsigned char c = run.StartIcon;

    for (int j = 0; j < run.Count; j++)
    {
      RRE.drawChar(xPos, yPos, c);
      xPos += 16;
      // Serial.print(c, HEX);
      // Serial.print(" ");
      c++;
    }
    // Serial.println();
  }
}