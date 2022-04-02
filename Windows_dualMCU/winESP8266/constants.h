#define VER "ESP8266_V1.6"
#define DEV_NAME "WEMOS_mini"
#define JSON_SERIAL_SIZE 250
#define COMM_SERIAL Serial

char msgKW[4][7] = {"from", "type", "i", "i_ext"};
const char *msgTypes[] = {"act", "info", "error"};
const char *winStates[] = {"off", "up", "down", "Error"};
const char *msgErrs[] = {"Comm", "Parameters", "Boot", "unKnown-error"};
const char *msgInfo[] = {"status", "query", "boot_p", "Boot", "error", "button", "MQTT", "ping", "Ext_button"};
const char *msgAct[] = {winStates[0], winStates[1], winStates[2], winStates[3], "reset_MCU", "Auto-Off", "lockdown_on", "lockdown_off"};

char ext_topic[] = {"myHome/Windows/lockdown"};

enum sys_states : const uint8_t
{
    WIN_STOP,
    WIN_UP,
    WIN_DOWN,
    WIN_ERR,
};