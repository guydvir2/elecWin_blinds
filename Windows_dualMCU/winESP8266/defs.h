#define VER "ESP8266_V1.6"
#define JSON_SERIAL_SIZE 250
#define DEV_NAME "WEMOS_mini"
char *ext_topic = "myHome/Windows/lockdown";
char *sketch_paramfile = "/sketch_param.json";

enum sys_states : const uint8_t
{
    WIN_STOP,
    WIN_UP,
    WIN_DOWN,
    WIN_ERR,
};