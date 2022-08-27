#include <Arduino.h>

#define VER "Arduino_v1.8"
#define MCU_TYPE "ProMini"
#define DEV_NAME "MCU"

#define RELAY_ON LOW
#define SW_PRESSED LOW
#define SW_DOWN_PIN 2    /* Switch1 INPUT to Arduino */
#define SW_UP_PIN 3      /* Switch1 INPUT to Arduino */
#define SW2_DOWN_PIN 4   /* Switch2 INPUT to Arduino */
#define SW2_UP_PIN 5     /* Switch2 INPUT to Arduino */
#define SW3_DOWN_PIN 6   /* Switch3 INPUT to Arduino */
#define SW3_UP_PIN 7     /* Switch3 INPUT to Arduino */
#define REL_DOWN_PIN 10  /* OUTUPT to relay device */
#define REL_UP_PIN 11    /* OUTUPT to relay device */
#define REL2_DOWN_PIN 12 /* OUTUPT to relay device */
#define REL2_UP_PIN 13   /* OUTUPT to relay device */

#define DEBUG_MODE false
#define JSON_SERIAL_SIZE 200
#define TZ_Asia_Jerusalem PSTR("IST-2IDT,M3.4.4/26,M10.5.0")

const uint8_t rel_up_pins[2] = {REL_UP_PIN, REL2_UP_PIN};
const uint8_t rel_down_pins[2] = {REL_DOWN_PIN, REL2_DOWN_PIN};
const uint8_t sw_up_pins[2] = {SW_UP_PIN, SW2_UP_PIN};
const uint8_t sw_down_pins[2] = {SW_DOWN_PIN, SW2_DOWN_PIN};

#define WIN_OFF(x)                             \
    digitalWrite(rel_down_pins[x], !RELAY_ON); \
    digitalWrite(rel_up_pins[x], !RELAY_ON);
#define WIN_UP(x) \
    WIN_OFF(x);   \
    delay(10);    \
    digitalWrite(rel_up_pins[x], RELAY_ON);
#define WIN_DOWN(x) \
    WIN_OFF(x);     \
    delay(10);      \
    digitalWrite(rel_down_pins[x], RELAY_ON);

enum sys_states : const uint8_t
{
    WIN_STOP,
    WIN_UP,
    WIN_DOWN,
    WIN_ERR,
};

void (*resetFunc)(void) = 0;