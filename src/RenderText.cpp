#include "RenderText.h"
#include <Screen.h>
#include <rre_5x8.h>
// #include <rre_7x12.h>
#include <rre_fixed_8x16.h>
#include "CustomIcons.16x16.h"

RREFont RRE;

int TextXPos = 0;
int TextYPos = 0;
int TextLineHeight = 0;

// Needed for RREFont library initialization - defines our displays fillRect equivalent
void RRERect(int x, int y, int width, int height, int colour)
{
  Display.fillRect(x, y, width, height, colour);
}

