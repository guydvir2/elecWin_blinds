#include <Arduino.h>

// ~~~~ Services update via ESP on BOOT ~~~~~
bool DualSW = false;     /* 2 Switches Window*/
bool Err_Protect = true; /* Monitor UP&DOWN pressed together*/
bool AutoOff = true;     /* Timeout to switch off Relays */
bool Lockdown = false;   /* lock operations of relays, both MQTT and Switch */

uint8_t AutoOff_duration = 60;
uint8_t btype_2 = 2; // Button Type
time_t bootTime;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
const uint8_t MIN2RESET_BAD_P = 30; /* Minutes to reset due to not getting Remote Parameters */

bool getP_OK = false; /* Flag, external parameters got OK ? */

void _update_bootP(JsonDocument &_doc)
{
    Err_Protect = _doc["err_p"];
    DualSW = _doc["dub_sw"];
    AutoOff = _doc["t_out"];
    AutoOff_duration = _doc["t_out_d"];
    bootTime = _doc["boot_t"].as<time_t>();
    Lockdown = _doc["Lockdown"];
    btype_2 = _doc["btype_2"]; /* Button type for external input only. This part is not solved yet */

    getP_OK = true;
}
void _ask_bootP()
{
    sendMSG(msgTypes[1], msgInfo[2]); /* calling for remote parameters */
}
void request_remoteParameters(uint8_t _waitDuration = 15)
{
    long last_req = millis();
    _ask_bootP();
    while (millis() < _waitDuration * 1000 && getP_OK == false) /* Wait to get parameters */
    {
        if (millis() - last_req > 2000) /* ask again */
        {
            last_req = millis();
            _ask_bootP();
        }
        readSerial();
        delay(50);
    }
}
void reset_fail_load_parameters()
{
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
        sendMSG(msgTypes[2], msgInfo[3], "NTP");
    }
    if (getP_OK == false)
    {
        sendMSG(msgTypes[2], msgInfo[3], "Parameters");
    }
}