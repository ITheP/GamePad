#include "RenderText.h"
#include <Screen.h>
#include <rre_5x8.h>
// #include <rre_7x12.h>
#include <rre_fixed_8x16.h>
#include "CustomIcons.16x16.h"

RREFont RRE;
RREFont RREDefault;
RREFont RRECustom;
RREFont RRESmall;

int TextXPos = 0;
int TextYPos = 0;
int TextLineHeight = 0;


void setupRRE()
{
  RRE.init(RRERect, SCREEN_WIDTH, SCREEN_HEIGHT);
  RRE.setCR(0);
  RRE.setScale(1);
  SetFontFixed();
  //SetFontLineHeightFixed();

  RREDefault.init(RRERect, SCREEN_WIDTH, SCREEN_HEIGHT);
  RREDefault.setCR(0);
  RREDefault.setScale(1);
  RREDefault.setFont(&rre_fixed_8x16);

  RRECustom.init(RRERect, SCREEN_WIDTH, SCREEN_HEIGHT);
  RRECustom.setCR(0);
  RRECustom.setScale(1);
  RRECustom.setFont(&rre_CustomIcons16);

  RRESmall.init(RRERect, SCREEN_WIDTH, SCREEN_HEIGHT);
  RRESmall.setCR(0);
  RRESmall.setScale(1);
  RRESmall.setFont(&rre_5x8);
}

// Needed for RREFont library initialization - defines our displays fillRect equivalent
void RRERect(int x, int y, int width, int height, int colour)
{
  Display.fillRect(x, y, width, height, colour);
}

// Much juicier (but slower) text rendering than inline variants in . h file
void PrintDisplayLine(const TextLine *line)
{
  RRE.printStr(TextXPos, TextYPos, (char *)line->text);
  TextYPos += TextLineHeight;
}

