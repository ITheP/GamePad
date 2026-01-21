(c) 2025-2026 MisterB @ ITheP

**Pre-Alpha**

***Not ready for use or release. Not for public use.***

## About
Embedded esp32-s3/arduino/equivalent controller. Originally written to convert old Guitar Hero controllers to wireless windows/android compatible bluetooth controllers.

## Features
- **Bluetooth Gamepad**: Connects to Windows and Android devices via Bluetooth. Allows for multiple bluetooth identities off a single controller.
- **LED Effects**: Supports various LED effects for visual feedback.
- **Display**: Screen showing controller state and information with visuals.
- **Stats**: Capture controller usage statistics
- **Configurable**: Configure at low level different controllers from config files
## License
TBC - At the moment this is not licensed for use.

## Documentation
When more detailed documentation gets done, it will be in the [Wiki](https://github.com/ITheP/GamePad/wiki).

## Contributing
Not yet!

## Contact
For any questions or feedback, feel free to contact MisterB at [ITheP](https://github.com/ITheP/GamePad/wiki).

## To Do Lists

- [ ] Separate Logo/Controller gfx files for different types, #include ones relevant to controller type...
- [x] OLED screen support
- [x] Custom icons
- [x] Battery life indicator
- [x] Low battery override warning screen
- [x] Bluetooth controller support
- [x] Bluetooth state (searching for connection/connected)
- [x] Support multiple bluetooth id's on a single controller (controlled at boot time)
- [x] Automagic Bluetooth device naming
- [x] Custom Bluetooth device name
- [x] Automatic unique Bluetooth Id's
- [x] Controller visual display with live control state
- [x] Basic stats (strums per second, total strums)
- [x] Startup information
- [x] Benchmarking for performance
- [x] Extra debug information
- [x] Support for multiple controllers (via configuration files)
- [x] Better stats calculation with better decoupling of hardcoded stats counting
- [ ] Wired gamepad support
- [x] User selectable configuration in controller
- [x] Scrollable help text in controller configuration
- [x] Basic text style formatting of Help text
- [x] Save user config to flash
- [x] Save user stats to flash
- [ ] Query around power brownout on off and saving
- [ ] Save to flash encryption
- [ ] Hiding of existing password (show 1st/last chars only) - UI specific, don't overwrite saved passwords
- [ ] Default Device Names from default Profiles
- [x] Trigger on button down, and button up, and alternative long press triggers (so can e.g. hold button to access menus while retaining its ability to be used in game)
- [ ] One effect can affect others - e.g. buttons held down affect colour of another effect
- [ ] HAT code needs tweaking to finish it completely
- [ ] Impliment analog stick support
- [ ] Add more stats tracking
- [ ] Add alternative screens (for more detailed stats etc.) though possibly not needed with web server
- [ ] Inactivity screen blanking/LED blanking [power saving]
- [ ] Activity screen blanking (unlikely to see screen if playing) [power saving]
- [ ] Test/debug screen - show pins etc. on button presses (access via boot option?)
- [ ] Investigate ESP32-S3 cpu throttling (reduce mhz) power saving mode
- [ ] Settings using whammy bar - e.g. hold button to set brightness and wiggle whammy bar to set it
- [ ] Menu option to auto-re-boot into configuration menus
- [ ] Statistics menu item, show changing every x seconds random stats. Possibly when scroll ends it updates with content from a new scroll (2*scroll text move from one to other)
- [ ] Controller idle - on display idle keep to max ?3? pixel for first ?20? seconds in case actually in game and don't want performance drop of long term idle effect overhead.
- [ ] Shift logos out of Icons font into own Logo font to free up icon space, and devices font for device images
- [x] Separate LED and Screen idle triggers and timings (allows for different overheads to affect the device separately)

## To Do - Config & Prefs
- [x] User selectable configuration in web
- [x] Controller config WiFi access point selection
- [x] Controller config WiFi set password
- [ ] Controller config set custom Device Name
- [ ] Analog inputs - have a configuration screen that allows you to set min/max values (by using them) and save to settings
- [ ] Use same WiFi details as Default (makes multiple profiles easier while specifying single WiFi). Menus should should dynamically change when this is enabled (don't show access point selection or password)
- [ ] No WiFi? Make sure not powered.
- [ ] Config current profile via built in access point menu option

## To Do - Web Config & Prefs
- [ ] Preferences/settings page in Web site
- [ ] Profile editor in Web Site
- [ ] Custom device name (also remove current hardcoded custom device name, won't be needed)
- [ ] Invert display (black on white)
- [ ] Default all profiles to using Default profile WiFi
- [ ] Flip-able controls - i.e. flips mappings inside so e.g. up and down flipped on strum bar (or whole HAT is flipped and upside down)
- [ ] Icons for Profiles
- [ ] Choose different guitar/controller gfx
- [ ] Set controller boot speed - slow (twice normal delay), normal (standard boot default delays), fast (remove all delays)

## To Do - Networking
- [x] WiFi support
- [ ] No WiFi? Make sure not powered.

## To Do - Web
- [x] Embedded web service

## To Do - Profiles
- [x] Multiple profiles
- [x] Controller config profile handling
- [x] Controller config profile copy/paste

## To Do - Battery
- [ ] More accurate battery life indicator
- [ ] When h/w allows, initial live battery monitoring loop checks for battery level - and if charging, and level high enough, will then exit on past the loop and allow device to continue booting.
- [ ] Charging battery status (when hardware allows for it) - flashing when charging, not flashing when plugged in by charged

## To Do - LED
- [x] Startup LED tests
- [x] Onboard LED
- [x] Neopixel/equivalent RGB external LEDs
- [ ] Support for more LED effects
- [x] Idle LED effects
- [ ] Independent LED effects (e.g. based on Stats)
- [ ] Run time configurable LEDs (disable if power below certain level, turn on/off in options)
- [ ] If External LED's powered on when empty battery kicks in - turn them all off to preserve battery power
  
## To Do - Electronics

- [x] Battery level monitoring
- [x] Integration with buttons
- [X] Integration with HATs (Digital D-Pad)
- [X] Integration with Analog inputs (whammy bar)
- [X] Connect to external (to device) LEDs
- [ ] LED's that only power up when USB connected
- [ ] USB charging while playing from USB power (hot plug-able while playing)
- [ ] Dual battery power (possibly one dedicated to device so isn't effected by LEDs)
- [ ] Include SPI variant of OLED screen. Currently using I2C, limits max FPS to around 48 due to I2C interface limitations. SPI uses more pins but can push 900+fps - i.e. far lower device load and so faster response.
- [ ] Include digital and analog input matrix to allow for far more inputs than pins on esp32-s3 dev module

## To Do - Other

- [ ] Stop writing code so much and actually impliment/convert some more physical controllers to use it!


