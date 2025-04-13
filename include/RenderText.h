#pragma once

#include <RREFont.h>

extern RREFont RRE;
extern RRE_Font rre_CustomIcons16;
extern RRE_Font rre_fixed_8x16;

static const byte RREHeight_fixed_8x16 = 14;

extern int TextXPos;
extern int TextYPos;

void RRERect(int x, int y, int width, int height, int colour);

inline void SetFontFixed()
{
  RRE.setFont(&rre_fixed_8x16);
}

inline void SetFontCustom()
{
  RRE.setFont(&rre_CustomIcons16);
}

extern char buffer[64];

// XPos should specify centre render point
inline void PrintCenteredNumber(int xpos, int ypos, unsigned long val)
{
  itoa(val, buffer, 10);
  xpos -= (RRE.strWidth(buffer) >> 1);

  RRE.printStr(xpos, ypos, buffer);
}

// For very basic automated text layout on screen

inline void ResetPrintDisplayLine()
{
  TextXPos = 0;
  TextYPos = 0;
}

inline void PrintDisplayLine(char *text)
{
  RRE.printStr(TextXPos, TextYPos, text);
  TextYPos += RREHeight_fixed_8x16;
}

inline void PrintDisplayLineCenter(char *text)
{
  RRE.printStr(ALIGN_CENTER, TextYPos, text);
  TextYPos += RREHeight_fixed_8x16;
}

inline void PrintDisplayLineRight(char *text)
{
  RRE.printStr(ALIGN_RIGHT, TextYPos, text);
  TextYPos += RREHeight_fixed_8x16;
}