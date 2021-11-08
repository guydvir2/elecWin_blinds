#include <Arduino.h>

#define DEV_NAME "WEMOS_mini"
#define JSON_SIZE_IOT 400
#define JSON_SIZE_SKETCH 300
#define JSON_SERIAL_SIZE 250
#define VER "ESP8266_V1.4"

enum sys_states : const uint8_t
{
    WIN_ERR,
    WIN_UP,
    WIN_DOWN,
    WIN_STOP,
};