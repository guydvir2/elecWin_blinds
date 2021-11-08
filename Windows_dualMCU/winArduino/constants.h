#include <Arduino.h>

#define VER "Arduino_v1.62"
#define MCU_TYPE "ProMini"
#define DEV_NAME "MCU"

#define RELAY_ON LOW
#define SW_PRESSED LOW
#define SW_DOWN_PIN 2   /* Switch1 INPUT to Arduino */
#define SW_UP_PIN 3     /* Switch1 INPUT to Arduino */
#define SW2_DOWN_PIN 4  /* Switch2 INPUT to Arduino */
#define SW2_UP_PIN 5    /* Switch2 INPUT to Arduino */
#define REL_DOWN_PIN 10 /* OUTUPT to relay device */
#define REL_UP_PIN 11   /* OUTUPT to relay device */

#define DEBUG_MODE false
#define JSON_SERIAL_SIZE 200

enum sys_states : const uint8_t
{
    WIN_ERR,
    WIN_UP,
    WIN_DOWN,
    WIN_STOP,
};

void (*resetFunc)(void) = 0;