#pragma once

// Some standard values used all over the place

#define LOW 0x0
#define HIGH 0x1

#define ON 0x0
#define OFF 0x1

#define PRESSED 0x0
#define NOT_PRESSED 0x1
#define LONG_PRESS_MONITORING 0x2
#define LONG_PRESS 0x3

#define HAT_POS_CENTRED 0x0
#define HAT_POS_UP 0x1
#define HAT_POS_UP_RIGHT 0x2
#define HAT_POS_RIGHT 0x3
#define HAT_POS_DOWN_RIGHT 0x4
#define HAT_POS_DOWN 0x5
#define HAT_POS_DOWN_LEFT 0x6
#define HAT_POS_LEFT 0x7
#define HAT_POS_UP_LEFT 0x8

enum ControllerReport {
    ReportToController = 0,
    DontReportToController = 1
};

#define NONE 0