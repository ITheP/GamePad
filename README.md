(c) 2025 MisterB @ ITheP

**Pre-Alpha**

***Not ready for use or release. Not for public use.***

## About
Embedded esp32-s3/arduino/equivalent game controller. Originally written to convert old Guitar Hero controllers to wireless windows/android compatible bluetooth game controllers.

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

## To Do - Firmware

Includes...


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
- [x] Onboard LED
- [x] Neopixel/equivalent RGB external LEDs
- [x] Controller visual display with live control state
- [x] Basic stats (strums per second, total strums)
- [x] Startup information and LED tests
- [x] Benchmarking for performance
- [x] Extra debug information
- [x] Support for multiple controllers (via configuration files)
<<<<<<< HEAD
- [ ] Better stats calculation with better decoupling of hardcoded stats counting
=======
>>>>>>> 37d665f81b376b326bc3eb9f5451e1321a663ce3
- [ ] Wired gamepad support
- [ ] Run time user selectable options in controller
- [ ] Save user options (and stats) to I2C Non-Volatile FRAM
- [ ] Save user options (and stats) to onboard Flash (note: during power brownout on off)
- [ ] Support for more LED effects
- [ ] Idle LED effects
- [ ] Run time configurable LEDs (disable if power below certain level, turn on/off in options)
- [ ] More accurate battery life indicator
- [ ] HAT code needs tweaking to finish it completely
- [ ] Impliment analog stick support
- [ ] Add more stats tracking
- [ ] Add alternative screens (for more detailed stats etc.)
- [ ] Inactivity screen blanking/LED blanking [power saving]
- [ ] Activity screen blanking (unlikely to see screen if playing) [power saving]
- [ ] Test/debug screen - show pins etc. on button presses (access via boot option?)
- [ ] Investigate ESP32-S3 cpu throttling (reduce mhz) power saving mode
  
## To Do - Electronics

- [x] Battery level monitoring
- [x] Integration with buttons
- [X] Integration with HATs (Digital D-Pad)
- [X] Integration with Analog inputs (whammy bar)
- [X] Connect to external (to device) LEDs
- [ ] LED's that only power up when USB connected
- [ ] USB charging while playing from USB power (hot plug-able while playing)
- [ ] Dual battery power (possibly one dedicated to device so doesn't touch LEDs)

## To Do - Other

- [ ] Stop writing code so much and actually impliment/convert some more physical controllers to use it!


