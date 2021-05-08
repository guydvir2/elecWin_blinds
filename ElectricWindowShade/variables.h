#include <ArduinoJson.h>

// ********** Sketch Services  ***********
#define VER "WEMOS_7.0"
#define RelayOn LOW
#define SwitchOn LOW
#define USE_BOUNCE_DEBUG false
#define JSON_SIZE_IOT 400
#define JSON_SIZE_SKETCH 300

bool useExtInput;
bool useAutoRelayOFF;

int AutoRelayOff_timeout;
const int deBounceInt = 50;
unsigned long autoOff_clock = 0;

//~~~~ Switches IOs~~~~~~
int inputUpPin;
int inputDownPin;
int outputUpPin;
int outputDownPin;
int inputUpExtPin;
int inputDownExtPin;

// GPIO status flags
bool inputUp_lastState;
bool inputDown_lastState;
bool inputUpExt_lastState;
bool inputDownExt_lastState;

