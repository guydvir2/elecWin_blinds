#include <Arduino.h>

// ~~~~ Services update via ESP on BOOT ~~~~~
bool DualSW = false;     /* 2 Switches Window - only for Windows_0 */
bool AutoOff = true;     /* Timeout to switch off Relays */
bool Lockdown = false;   /* lock operations of relays, both MQTT and Switch */
uint8_t numWindows = 1;
uint8_t AutoOff_duration = 60;
uint8_t btype_2 = 2; // Button Type
time_t bootTime;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool getP_OK = false; /* Flag, external parameters got OK ? */

void _update_bootP(JsonDocument &_doc)
{
    DualSW = _doc["dub_sw"];
    numWindows = _doc["numWindows"];
    AutoOff = _doc["t_out"];
    AutoOff_duration = _doc["t_out_d"];
    bootTime = _doc["boot_t"].as<time_t>();
    Lockdown = _doc["Lockdown"];
    btype_2 = _doc["btype_2"]; /* Button type for external input only. This part is not solved yet */

    getP_OK = true;
}
void _send_P_request()
{
    sendMSG(msgTypes[1], msgInfo[2]); /* calling for remote parameters */
}
void request_remoteParameters(uint8_t _waitDuration = 15)
{
    long last_req = millis();
    _send_P_request();
    while (millis() < _waitDuration * 1000 && getP_OK == false) /* Wait to get parameters */
    {
        if (millis() - last_req > 2000) /* ask again */
        {
            last_req = millis();
            _send_P_request();
        }
        readSerial();
        delay(50);
    }
}
void reset_fail_load_parameters()
{
    const uint8_t MIN2RESET_BAD_P = 30; /* Minutes to reset due to not getting Remote Parameters */
    if (getP_OK == false && millis() > MIN2RESET_BAD_P * 60000UL)
    {
        sendMSG(msgTypes[2], msgErrs[1]);
        delay(1000);
        resetFunc();
    }
}
void postBoot_err_notification()
{
    sendMSG(msgTypes[1], msgInfo[3]);
    struct tm *tm = localtime(&bootTime);

    if (tm->tm_year == 70)
    {
        sendMSG(msgTypes[2], msgInfo[3], "NTP"); /* Error time update */
    }
    if (getP_OK == false)
    {
        sendMSG(msgTypes[2], msgInfo[3], "Parameters"); /* Error receiving Boot parameters */
    }
}