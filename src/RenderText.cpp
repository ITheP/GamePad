#include "RenderText.h"
#include <Screen.h>
#include <rre_5x8.h>
// #include <rre_7x12.h>
#include <rre_fixed_8x16.h>
#include "CustomIcons.16x16.h"
#include <IconMappings.h>

RREFont RRE;
RREFont RREDefault;
RREFont RREIcon;
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

  RREDefault.init(RRERect, SCREEN_WIDTH, SCREEN_HEIGHT);
  RREDefault.setCR(0);
  RREDefault.setScale(1);
  RREDefault.setFont(&rre_fixed_8x16);

  RREIcon.init(RRERect, SCREEN_WIDTH, SCREEN_HEIGHT);
  RREIcon.setCR(0);
  RREIcon.setScale(1);
  RREIcon.setFont(&rre_CustomIcons16);

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

// Much juicier (but slower) text rendering than inline variants in .h file.
void PrintDisplayLine(const TextLine *line)
{
  int textPos = TextXPos;
  char *text = (char *)line->text;
  TextStyle style = (TextStyle)line->style;
  uint8_t icon = line->Icon;

  // We have to manually calculate centering etc. points
  // just in case we need to make bold etc.

  if (style & STYLE_CENTRED)
  {
    textPos = (SCREEN_WIDTH - textPos) >> 1; // Centre point of screen, accounting for possible indentation
    textPos -= RRE.strWidth(text) >> 1;      // and then offset half the width of the text
  }
  else if (style & STYLE_ALIGNRIGHT)
    textPos = SCREEN_WIDTH - RRE.strWidth(text);

  if (style & STYLE_SEPARATOR)
    Display.drawFastHLine(0, TextYPos + (TextLineHeight >> 1) - 2, SCREEN_WIDTH, C_WHITE);

  if (style & STYLE_UNDERLINE)
    Display.drawFastHLine(0, TextYPos + TextLineHeight - 2, RRE.strWidth(text), C_WHITE);

  if (style & STYLE_H1)
  {
    if (icon != 0)
    {
      // Icons are drawn and width calculated
      RREIcon.drawChar(0, TextYPos -1, icon);
      if (textPos == 0)
        textPos += 16;
    }

    RRE.printStr(textPos, TextYPos, text);
    RRE.printStr(textPos + 1, TextYPos, text);
    RRE.printStr(textPos, TextYPos + 1, text);
    RRE.printStr(textPos + 1, TextYPos + 1, text);

    TextYPos += TextLineHeight + 1; // +1 for the extra boldness
  }
  else if (style & STYLE_HIGHLIGHT)
  {
    // Draw white rectangle and then print text in black
    int w = RRE.strWidth(text);
    Display.fillRect(textPos, TextYPos, w + 2, TextLineHeight + 1, C_WHITE);
    RRE.setColor(C_BLACK);
    RRE.printStr(textPos + 1, TextYPos + 1, text);
    RRE.setColor(C_WHITE);
    TextYPos += TextLineHeight + 2; // +2 for highlight top/bottom
  }
  else
  {
    // Check for an Icon or bullet
    if (style & STYLE_BULLET)
    {
      // Bullets use Icon or a default if not set with a set indent
      // If Icon = -1 then we still treat as bullet
      // but don't draw any icon - allows for
      // indenting of a single bullet point text
      // over multiple lines
      if (icon != Icon_IGNORE) {
        char c = (icon == 0 ? Icon_FilledCircle_2 : icon);
        RREIcon.drawChar(textPos + 4, TextYPos + 3, c);
      }
      textPos += 14;
    }
    else if (icon != 0)
    {
      // Icons are drawn and width calculated
      RREIcon.drawChar(0, TextYPos -1, icon);
      if (textPos == 0)
        textPos += 14;
    }

    // Bold, draw extra offset by 1 pixel
    // To make clearer, replace with printing with different font
    if (style & STYLE_BOLD)
      RRE.printStr(textPos + 1, TextYPos, text);

    RRE.printStr(textPos, TextYPos, text);
    TextYPos += TextLineHeight;
  }
}
