#define VER "WEMOS_0.2"
#define RelayOn LOW
#define JSON_SIZE_IOT 400
#define JSON_SIZE_SKETCH 300

const byte WIN_STOP = 0;
const byte WIN_UP = 1;
const byte WIN_DOWN = 2;

const byte delay_switch = 100; //ms 10 times more than MCU's looper - to have enough time to react to change
const byte delay_loop = 200;   //ms

bool relayUP_lastState = !RelayOn;
bool relayDOWN_lastState = !RelayOn;

//~~~~ Switches IOs~~~~~~
int outputUpPin;
int outputDownPin;
int relayUpPin;
int relayDownPin;

bool useAutoOff = false;
byte autoOff_time = 30; //seconds
unsigned long autoOff_clk=0;
