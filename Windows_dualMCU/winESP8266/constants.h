#define VER "ESP8266_V1.6"
#define DEV_NAME "WEMOS_mini"
#define JSON_SERIAL_SIZE 256
#define COMM_SERIAL Serial

enum sys_states : const uint8_t
{
    WIN_STOP,
    WIN_UP,
    WIN_DOWN,
    WIN_ERR,
};

char ext_topic[] = {"myHome/Windows/lockdown"};

char msgTypes[3][7] = {"act", "info", "error"};
char msgKW[4][7] = {"from", "type", "i", "i_ext"};
char msgErrs[4][15] = {"Comm", "Parameters", "Boot", "unKnown-error"};
char msgInfo[9][12] = {"status", "query", "boot_p", "Boot", "error", "button", "MQTT", "ping", "Ext_button"};

const char *winStates[] = {"off", "up", "down", "Error"};
const char *msgAct[]= {winStates[0], winStates[1], winStates[2], winStates[3], "reset_MCU", "Auto-Off", "lockdown_on", "lockdown_off"};

