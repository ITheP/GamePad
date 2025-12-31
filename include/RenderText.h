#pragma once

#include <RREFont.h>

enum TextStyle : uint16_t {
    STYLE_NONE      = 0,
    STYLE_BOLD      = 1 << 0,   // TODO: Impliment
    STYLE_ITALIC    = 1 << 1,   // TODO: Impliment
    STYLE_UNDERLINE = 1 << 2,   // TODO: Impliment
    STYLE_CENTERED  = 1 << 3,
    STYLE_HIGHLIGHT = 1 << 4,  // Up to code how it want's to do this
    STYLE_H1        = 1 << 5,
    STYLE_H2        = 1 << 6,
    STYLE_SEPARATOR = 1 << 7,    // Horizontal separator line
    STYLE_BULLET    = 1 << 8    // Bullet point
};

struct TextLine {
    TextStyle style;
    const char* text;
};

// General RRE that general purpose text display might use - can be retargetted to different fonts so e.g.
// PrintDisplayLine() can render different font sizes
extern RREFont RRE;
extern RREFont RREDefault;
extern RREFont RRECustom;
extern RREFont RRESmall;

extern RRE_Font rre_CustomIcons16;
extern RRE_Font rre_5x8;
// extern RRE_Font rre_7x12;
extern RRE_Font rre_fixed_8x16;

static const byte RREHeight_5x8 = 10;
// static const byte RREHeight_7x12 = 12;
static const byte RREHeight_fixed_8x16 = 14;
static const byte RREHeight_Custom = 14;

extern int TextXPos;
extern int TextYPos;
extern int TextLineHeight;

void setupRRE();

void RRERect(int x, int y, int width, int height, int colour);

#define FONT_DEFAULT  &rre_fixed_8x16
#define FONT_SMALL    &rre_5x8
#define FONT_CUSTOM   &rre_CustomIcons16

inline void SetFontSmall()
{
  RRE.setFont(&rre_5x8);
  TextLineHeight = RREHeight_5x8;
}

inline void SetFontFixed()
{
  RRE.setFont(&rre_fixed_8x16);
  TextLineHeight = RREHeight_fixed_8x16;
}

inline void SetFontCustom()
{
  RRE.setFont(&rre_CustomIcons16);
  TextLineHeight = RREHeight_Custom;
}

extern char buffer[128];

// XPos should specify centre render point
inline void PrintCenteredNumber(int xpos, int ypos, unsigned long val)
{
  itoa(val, buffer, 10);
  xpos -= (RRE.strWidth(buffer) >> 1);

  RRE.printStr(xpos, ypos, buffer);
}

// For very basic automated text layout on screen

inline void ResetPrintDisplayLine(int yOffset = 0, int xOffset = 0, void (*fontFunc)() = SetFontFixed)
{
  TextXPos = xOffset;
  TextYPos = yOffset;

  fontFunc();
}

inline void PrintDisplayLine(char *text)
{
  RRE.printStr(TextXPos, TextYPos, text);
  TextYPos += TextLineHeight;
}

inline void PrintDisplayLineCentered(char *text)
{
  RRE.printStr(ALIGN_CENTER, TextYPos, text);
  TextYPos += TextLineHeight;
}

inline void PrintDisplayLineRight(char *text)
{
  RRE.printStr(ALIGN_RIGHT, TextYPos, text);
  TextYPos += TextLineHeight;
}

void PrintDisplayLine(const TextLine *line);