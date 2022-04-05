#define VER "Arduino_v1.6"
#define MCU_TYPE "ProMini"
#define JSON_SERIAL_SIZE 256
#define DEV_NAME "MCU"

#define RELAY_ON LOW
#define SW_PRESSED LOW
#define COMM_SERIAL Serial

#define SW_DOWN_PIN 2   /* Switch1 INPUT to Arduino */
#define SW_UP_PIN 3     /* Switch1 INPUT to Arduino */
#define SW2_DOWN_PIN 4  /* Switch2 INPUT to Arduino */
#define SW2_UP_PIN 5    /* Switch2 INPUT to Arduino */
#define REL_DOWN_PIN 10 /* OUTUPT to relay device */
#define REL_UP_PIN 11   /* OUTUPT to relay device */


enum sys_states : const uint8_t
{
    WIN_STOP,
    WIN_UP,
    WIN_DOWN,
    WIN_ERR,
};

void (*resetFunc)(void) = 0;
char msgKW[4][7] = {"from", "type", "i", "i_ext"};
char msgTypes[4][7] = {"act", "info", "error", "system"};
char msgErrs[4][15] = {"Comm", "Parameters", "Boot", "unKnown-error"};
char msgInfo[9][12] = {"status", "query", "boot_p", "Boot", "error", "button", "MQTT", "ping", "Ext_button"};

char *winStates[] = {"off", "up", "down", "Error"};
const char *msgAct[] = {winStates[0], winStates[1], winStates[2], winStates[3], "reset_MCU", "Auto-Off", "lockdown_on", "lockdown_off"};
