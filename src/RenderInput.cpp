//#include <Adafruit_SH110X.h>
#include <RREFont.h>
#include "Config.h"
#include "GamePad.h"
#include "Structs.h"
#include "Screen.h"

// Draws a rectangle depending on value of Input (e.g. button press/release)
void RenderInput_Rectangle(Input* input) {
  int color;
  if (input->ValueState.Value == HIGH) {
    color = C_BLACK;

#ifdef EXTRA_SERIAL_DEBUG
    Serial.println("Rec.Button up [" + String(input->BluetoothInput) + "]: " + String(input->Label));
#endif
  } else {
    color = C_WHITE;

#ifdef EXTRA_SERIAL_DEBUG
    Serial.println("Rec.Button down [" + String(input->BluetoothInput) + "]: " + String(input->Label));
#endif
  }

  Display.fillRect(input->XPos, input->YPos, input->RenderWidth, input->RenderHeight, color);
}

// Draws specific icons depending on state of input
// Assumes icon font is currently selected
void RenderInput_Icon(Input* input) {
  int xPos = input->XPos;
  int yPos = input->YPos;
  int width = input->RenderWidth;
  int height = input->RenderHeight;

  // Blank existing icon
  Display.fillRect(xPos, yPos, width, height, C_BLACK);

  if (input->ValueState.Value == HIGH) {
#ifdef EXTRA_SERIAL_DEBUG
    Serial.println("Icon.Button up [" + String(input->BluetoothInput) + "]: " + String(input->Label));
#endif

    // Only draw if required - otherwise we might just be blanking previous icon
    if (input->FalseIcon != 0)
      RRE.drawChar(input->XPos, input->YPos, input->FalseIcon);
  } else {
#ifdef EXTRA_SERIAL_DEBUG
    Serial.println("Icon.Button down [" + String(input->BluetoothInput) + "]: " + String(input->Label));
#endif

    // Only draw if required - otherwise we might just be blanking previous icon
    if (input->TrueIcon != NONE)
      RRE.drawChar(input->XPos, input->YPos, input->TrueIcon);
  }
}

// Renders icon + second icon next to it (i.e. double width)
// Assumes icon font is currently selected
void RenderInput_DoubleIcon(Input* input) {
  int xPos = input->XPos;
  int yPos = input->YPos;
  int width = input->RenderWidth;
  int height = input->RenderHeight;

  Display.fillRect(xPos, yPos, width, height, C_BLACK);

  if (input->ValueState.Value == HIGH) {
#ifdef EXTRA_SERIAL_DEBUG
    Serial.println("Icon.Button up [" + String(input->BluetoothInput) + "]: " + String(input->Label));
#endif

    if (input->FalseIcon != NONE) {
      RRE.drawChar(input->XPos, input->YPos, input->FalseIcon);
      RRE.drawChar(input->XPos + 16, input->YPos, input->FalseIcon + 1);
    }
  } else {
#ifdef EXTRA_SERIAL_DEBUG
    Serial.println("Icon.Button down [" + String(input->BluetoothInput) + "]: " + String(input->Label));
#endif

    if (input->TrueIcon != NONE)
      RRE.drawChar(input->XPos, input->YPos, input->TrueIcon);

    RRE.drawChar(input->XPos + 16, input->YPos, input->TrueIcon + 1);
  }
}

// ToDo: Renders text output
// assumes text font is currently selected
void RenderInput_Text(Input* input) {
#ifdef EXTRA_SERIAL_DEBUG
  Serial.println(input->Label);
#endif

  // CURRENTLY UNUSED
}

// Draws a rectangle, size varies by inputs value
void RenderInput_AnalogBar_Vert(Input* input) {
  int xPos = input->XPos;
  int yPos = input->YPos;
  int width = input->RenderWidth;
  int height = input->RenderHeight;

// Renders a vertical bar, from bottom to top
#ifdef EXTRA_SERIAL_DEBUG
  Serial.println("AnalogBar." + String(input->Label) + ".Change: " + String(input->Value));
#endif

  // Clear any old bar
  Display.fillRect(xPos, yPos, width, height, C_BLACK);

  int barHeight = (input->ValueState.Value / 4096.0) * height;
  // Make sure bar is always at least 1 pixel high, and in our case here account for fractional rounding down to nearest pixel (which might otherwise cause a full value to miss final pixel of height)
  // Note that for some reason, .fillRect was creating a 2 pixel high rectangle with a funny offset when barHeight was set to 0
  barHeight++;

  Display.fillRect(xPos, yPos + (height - barHeight), width, barHeight, C_WHITE);
}

// Renders a hat up/down/left/right or diagonal position icon depending on input value
void RenderInput_Hat(HatInput* hatInput) {
  int xPos = hatInput->XPos;
  int yPos = hatInput->YPos;
  int width = hatInput->RenderWidth;
  int height = hatInput->RenderHeight;

#ifdef EXTRA_SERIAL_DEBUG
  Serial.println("HatInput." + String(hatInput->Label) + ".Change: " + String(hatInput->Value));
#endif

  // Clear previous
  Display.fillRect(xPos, yPos, width, height, C_BLACK);

  // Value should be 0 for neutral position (nothing selected). Other icons are just + the value as an offset
  RRE.drawChar(xPos, yPos, (hatInput->StartIcon + hatInput->ValueState.Value));
}