#pragma once

#include <RREFont.h>

// Styles are set per line (doing fancy per word/letter sub-text formatting on embedded device with slow display's
// meant sacrificing fancy formatting in exchange for simplicity and speed

// NOTE:
// Highlight don't work with Bold or Icons

enum TextStyle : uint16_t {
    STYLE_NONE      = 0,
    STYLE_BOLD      = 1 << 0,
    //STYLE_ITALIC    = 1 << 1,  // TODO: Impliment, probably need a separate font
    STYLE_UNDERLINE = 1 << 2,
    STYLE_CENTRED   = 1 << 3,
    STYLE_ALIGNRIGHT= 1 << 4,
    STYLE_HIGHLIGHT = 1 << 5,  // Inverse colour - doesn't work with BOLD
    STYLE_H1        = 1 << 6,  // H1 was going to be a bigger font, however scrolling calculations become more complicated. For now this does the equivalent of what H2 was going to do.
    STYLE_SEPARATOR = 1 << 7,  // Horizontal separator line
    STYLE_BULLET    = 1 << 8,  // Bullet point - set Icon to -1 to use bullet spacing but show no actual bullet
                               // (handy for indenting text across multiple lines for a single bullet point)
    STYLE_ICON      = 1 << 9   // First character will be printed using Custom Icon Font to the left
                               // This special case doesn't adjust other text positioning other than
                               // indenting text if rendered to the left
};

struct TextLine {
    int style;
    const char* text;
    uint8_t Icon;
};

// General RRE that general purpose text display might use - can be retargetted to different fonts so e.g.
// PrintDisplayLine() can render different font sizes
extern RREFont RRE;
extern RREFont RREDefault;
extern RREFont RREIcon;
extern RREFont RRESmall;

extern RRE_Font rre_CustomIcons16;
extern RRE_Font rre_5x8;
// extern RRE_Font rre_7x12;
extern RRE_Font rre_fixed_8x16;

static const byte RREHeight_5x8 = 10;
// static const byte RREHeight_7x12 = 12;
static const byte RREHeight_fixed_8x16 = 14;
static const byte RREHeight_Icon = 14;

extern int TextXPos;
extern int TextYPos;
extern int TextLineHeight;

void setupRRE();

void RRERect(int x, int y, int width, int height, int colour);

#define FONT_DEFAULT  &rre_fixed_8x16
#define FONT_DEFAULT_HEIGHT RREHeight_fixed_8x16

#define FONT_SMALL    &rre_5x8
#define FONT_SMALL_HEIGHT RREHeight_5x8

#define FONT_ICON   &rre_CustomIcons16
#define FONT_ICON_HEIGHT RREHeight_Icon

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

inline void SetFontIcon()
{
  RRE.setFont(&rre_CustomIcons16);
  TextLineHeight = RREHeight_Icon;
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