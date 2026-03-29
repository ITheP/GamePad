#include "Screen.h"
#include "Stats.h"
#include "Battery.h"
#include <LovyanGFX.hpp>

// Adafruit_SH1106G Display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Notes:
// When tested with mono oled 1288x64 display...
// I2C 4 pin display theoretical FPS maxes out around 48fps
// SPI 7 pin display theoretical FPS maxes out around 976fps

LGFX Display;

LGFX::LGFX()
{

  {
    auto cfg = _bus_instance.config();

    // SPI bus settings
    {
      /*

      // Select which SPI peripheral to use.
      // ESP32‑S2, C3: use SPI2_HOST or SPI3_HOST
      // ESP32: use VSPI_HOST or HSPI_HOST
      // Due to ESP‑IDF updates, VSPI_HOST and HSPI_HOST are becoming deprecated.
      // If you get errors, use SPI2_HOST or SPI3_HOST instead.
      cfg.spi_host = VSPI_HOST;

      cfg.spi_mode = 0;          // SPI communication mode 0 - 3
      cfg.freq_write = 40000000; // SPI clock speed for writing. Maximum 80 MHz. Values are rounded to an integer divisor of 80 MHz. Some devices can't cope with this speed, might need e.g. 40 or 55 MHz.
      cfg.freq_read = 16000000;  // SPI clock speed for reading
      cfg.spi_3wire = true;      // Set to true if receiving data via the MOSI pin (3‑wire SPI)
      cfg.use_lock = true;       // Set to true to use transaction locking

      // Set the DMA channel to use.
      // (0 = no DMA / 1 = channel 1 / 2 = channel 2 / SPI_DMA_CH_AUTO = automatic)
      // Due to ESP‑IDF updates, SPI_DMA_CH_AUTO (automatic) is now recommended.
      // Specifying channel 1 or 2 is becoming deprecated.
      cfg.dma_channel = SPI_DMA_CH_AUTO;

      cfg.pin_sclk = 18; // SPI SCLK pin number
      cfg.pin_mosi = 23; // SPI MOSI pin number
      cfg.pin_miso = 19; // SPI MISO pin number (-1 = disable)
      cfg.pin_dc = 27;   // SPI D/C pin number (-1 = disable)
                         // If sharing the SPI bus with an SD card, do NOT omit the MISO pin — it must be defined.
      */
    }

    // I2C bus settings
    {
      cfg.i2c_port = 0;              // Select which I2C port to use (0 or 1)
      cfg.freq_write = 400000;       // Clock speed for writing
      cfg.freq_read = 400000;        // Clock speed for reading
      cfg.pin_sda = I2C_SDA;         // Pin number connected to SDA
      cfg.pin_scl = I2C_SCL;         // Pin number connected to SCL
      cfg.i2c_addr = SCREEN_ADDRESS; // I2C device address
    }

    // 8‑bit parallel bus settings
    {
      /*
      cfg.i2s_port = I2S_NUM_0;  // Select which I2S port to use (I2S_NUM_0 or I2S_NUM_1)
                                 // (Uses ESP32's I2S LCD mode)
      cfg.freq_write = 20000000; // Write clock (max 20 MHz, rounded to an integer divisor of 80 MHz)
      cfg.pin_wr = 4;            // Pin number connected to WR
      cfg.pin_rd = 2;            // Pin number connected to RD
      cfg.pin_rs = 15;           // Pin number connected to RS (D/C)
      cfg.pin_d0 = 12;           // Pin number connected to D0
      cfg.pin_d1 = 13;           // Pin number connected to D1
      cfg.pin_d2 = 26;           // Pin number connected to D2
      cfg.pin_d3 = 25;           // Pin number connected to D3
      cfg.pin_d4 = 17;           // Pin number connected to D4
      cfg.pin_d5 = 16;           // Pin number connected to D5
      cfg.pin_d6 = 27;           // Pin number connected to D6
      cfg.pin_d7 = 14;           // Pin number connected to D7
      */
    }

    _bus_instance.config(cfg);              // 設定値をバスに反映します。
    _panel_instance.setBus(&_bus_instance); // バスをパネルにセットします。
  }

  {
    // Configure the display panel settings.
    auto cfg = _panel_instance.config(); // Retrieve the panel‑configuration structure.

    cfg.pin_cs = -1;   // SPI - Pin number connected to CS (Chip Select) (-1 = disable)      // 14
    cfg.pin_rst = -1;  // Pin number connected to RST (-1 = disable)      // 33
    cfg.pin_busy = -1; // Pin number connected to BUSY (-1 = disable)

    // The following settings already have reasonable defaults for each panel type.
    // If you are unsure about any item, try leaving it commented out.
    // cfg.panel_width = SCREEN_WIDTH;    // Actual visible width
    // cfg.panel_height = SCREEN_HEIGHT;   // Actual visible height
    // cfg.offset_x = 0;         // Horizontal offset of the panel
    // cfg.offset_y = 0;         // Vertical offset of the panel
    // cfg.offset_rotation = 0;  // Rotation offset value (0–7). 4–7 = upside‑down.
    // cfg.dummy_read_pixel = 8; // Number of dummy bits before pixel read operations
    // cfg.dummy_read_bits = 1;  // Number of dummy bits before non‑pixel read operations
    // cfg.readable = true;      // Set true if the panel supports reading data back
    // cfg.invert = false;       // Set true if the panel’s brightness appears inverted
    // cfg.rgb_order = false;    // Set true if red/blue channels appear swapped
    // cfg.dlen_16bit = false;   // Set true if the panel sends data in 16‑bit units (16‑bit SPI or parallel)
    // cfg.bus_shared = true;    // Set true if the bus is shared with an SD card
    //                           // (LovyanGFX will manage bus control for drawJpgFile, etc.)

    // Only set the following if the display appears shifted on drivers
    // like ST7735 or ILI9163 where the memory size differs from panel size.
    //    cfg.memory_width  = 240;   // Maximum width supported by the driver IC
    //    cfg.memory_height = 320;   // Maximum height supported by the driver IC

    // For our SH1106 I2C
    cfg.panel_width = SCREEN_WIDTH;   // Actual visible width
    cfg.panel_height = SCREEN_HEIGHT; // Actual visible height
    cfg.memory_width = SCREEN_MEMORY_WIDTH;
    cfg.memory_height = SCREEN_MEMORY_HEIGHT;
    cfg.offset_x = SCREEN_X_OFFSET; // Horizontal offset of the panel
    cfg.offset_y = SCREEN_Y_OFFSET; // Vertical offset of the panel
    cfg.offset_rotation = 0;        // Rotation offset value (0–7). 4–7 = upside‑down.

    _panel_instance.config(cfg);
  }

  // Configure the backlight control (if required)
  {
    /*
    auto cfg = _light_instance.config();   // Retrieve the backlight‑configuration structure.

    cfg.pin_bl = 32;        // Pin number connected to the backlight
    cfg.invert = false;     // Set true if the backlight brightness needs to be inverted
    cfg.freq = 44100;       // PWM frequency for the backlight
    cfg.pwm_channel = 7;    // PWM channel number to use

    _light_instance.config(cfg);
    _panel_instance.setLight(&_light_instance);   // Attach the backlight controller to the panel
    */
  }

  // Touch screen controls
  {
    /*
      // Configure the touchscreen control. (Delete this block if not needed)
      auto cfg = _touch_instance.config();

      cfg.x_min = 0;            // Minimum raw X value returned by the touchscreen
      cfg.x_max = 239;          // Maximum raw X value returned by the touchscreen
      cfg.y_min = 0;            // Minimum raw Y value returned by the touchscreen
      cfg.y_max = 319;          // Maximum raw Y value returned by the touchscreen
      cfg.pin_int = 38;         // Pin number connected to the INT (interrupt) line
      cfg.bus_shared = true;    // Set true if the touchscreen shares the same bus as the display
      cfg.offset_rotation = 0;  // Adjust if touch orientation does not match display (0–7)

      // For SPI connection
      cfg.spi_host = VSPI_HOST; // Select which SPI peripheral to use (HSPI_HOST or VSPI_HOST)
      cfg.freq = 1000000;       // SPI clock frequency
      cfg.pin_sclk = 18;        // Pin number connected to SCLK
      cfg.pin_mosi = 23;        // Pin number connected to MOSI
      cfg.pin_miso = 19;        // Pin number connected to MISO
      cfg.pin_cs   = 5;         // Pin number connected to CS

      // For I2C connection
      cfg.i2c_port = 1;         // Select which I2C port to use (0 or 1)
      cfg.i2c_addr = 0x38;      // I2C device address
      cfg.pin_sda  = 23;        // Pin number connected to SDA
      cfg.pin_scl  = 32;        // Pin number connected to SCL
      cfg.freq     = 400000;    // I2C clock frequency

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);   // Attach the touchscreen to the panel
      */
  }

  setPanel(&_panel_instance); // 使用するパネルをセットします。
};

// Screen orientation functions
int ScreenOrientation = 0;
int PreviousScreenOrientation = 0;

void FlipScreen(Input *input)
{
#ifdef LIVE_BATTERY
  // Don't draw if we are showing the end game battery warning
  if (Battery::PreviousBatteryLevel == 0)
  {
#ifdef EXTRA_SERIAL_DEBUG
    Serial.println("Flip Screen cancelled - BatteryLevel 0");
#endif
    return;
  }
#endif

  // #ifdef EXTRA_SERIAL_DEBUG
  //   Serial.println("Flip Screen " + String(input->ValueState.Value));
  // #endif

  // Set orientation will also need flipping

  // Note: Define FLIP_SCREEN_TOGGLE in controller config .h if you want a flip screen button to toggle each press, rather than a button hold to toggle screen orientation
  // .setRotation does not require Display.display() to be called

#ifdef FLIP_SCREEN_TOGGLE
  if (input->Value == HIGH)
    return;

  if (ScreenOrientation == 2)
    ScreenOrientation = 0;
  else
    ScreenOrientation = 2;

  Display.setRotation(ScreenOrientation);
#else
  if (input->ValueState.Value == HIGH)
    ScreenOrientation = 0;
  else
    ScreenOrientation = 2;

  Display.setRotation(ScreenOrientation);
#endif

  if (ScreenOrientation != PreviousScreenOrientation)
  {
    // Flip current content
    // Note that the main DrawMainScreen() is only partial in what it draws - doesn't include all the individual control points and dynamic values.
    // So we have a little (slow but functional) screen flipping routine to make sure what is already there is copied exactly as is, rather than clearing and redrawing the main screen.

    // We only need to cover 1/2 the screen, as it is swapping places with the other half.
    int height = SCREEN_HEIGHT >> 1;

    for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < SCREEN_WIDTH; x++)
      {
        // Read the pixel starting from the top-left corner
        int topPixel = Display.readPixel(x, y); // LovyanGFX - use .getPixel with Adafruit

        // Read the pixel we are swapping with from the bottom-right corner
        int bottomPixel = Display.readPixel(SCREEN_WIDTH - 1 - x, SCREEN_HEIGHT - 1 - y); // LovyanGFX - use .getPixel with Adafruit

        // Only drawing where there are actual differences saves a bit of time
        if (topPixel != bottomPixel)
        {
          // Draw the bottom pixel to the top position
          Display.drawPixel(x, y, bottomPixel);

          // Draw the top pixel to the bottom position
          Display.drawPixel(SCREEN_WIDTH - 1 - x, SCREEN_HEIGHT - 1 - y, topPixel);
        }
      }
    }
    // Update the display to show the changes
    Display.display();

    PreviousScreenOrientation = ScreenOrientation;
  }
}