// ~~~~ Services update via ESP on BOOT ~~~~~
time_t bootTime;
bool DualSW = false;     /* 2 Switches Window*/
bool AutoOff = true;     /* Timeout to switch off Relays */
bool Lockdown = false;   /* lock operations of relays, both MQTT and Switch */
bool Err_Protect = true; /* Monitor UP&DOWN pressed together*/
uint8_t btype_2 = 2; // Button Type
uint8_t AutoOff_duration = 60;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool getP_OK = false; /* Flag, external parameters got OK ? */

void _update_bootP(JsonDocument &_doc)
{
    DualSW = _doc["dub_sw"];
    AutoOff = _doc["t_out"];
    btype_2 = _doc["btype_2"]; /* Button type for external input only. This part is not solved yet */
    Lockdown = _doc["Lockdown"];
    AutoOff_duration = _doc["t_out_d"];
    bootTime = _doc["boot_t"].as<time_t>();

    getP_OK = true;
}
void _send_P_request()
{
    SerialComm.sendMsg(DEV_NAME,msgTypes[1], msgInfo[2]); /* calling for remote parameters */
}
void request_remoteParameters(uint8_t _waitDuration = 20)
{
    /* _waitDuration should be longer than 12 sec
    since at boot ESP8266 completes wake cycle around 10 sec 
    without any connection tolerances
     */
    unsigned long last_req = millis();
    _send_P_request();
    while (millis() < _waitDuration * 1000 && !getP_OK) /* Wait to get parameters */
    {
        if (millis() - last_req > 1000) /* ask again */
        {
            last_req = millis();
            _send_P_request();
        }
        SerialComm.loop();
        delay(50);
    }
}
void reset_fail_load_parameters()
{
    const uint8_t MIN2RESET_BAD_P = 10; /* Minutes to reset due to not getting Remote Parameters */
    if (!getP_OK && millis() > MIN2RESET_BAD_P * 60000UL)
    {
        SerialComm.sendMsg(DEV_NAME,msgTypes[2], msgErrs[1]);
        delay(1000);
        resetFunc();
    }
}
void postBoot_err_notification()
{
    SerialComm.sendMsg(DEV_NAME,msgTypes[1], msgInfo[3]);

    if (year(bootTime) == 1970)
    {
        SerialComm.sendMsg(DEV_NAME,msgTypes[2], msgInfo[3], "NTP");
    }
    if (getP_OK == false)
    {
        SerialComm.sendMsg(DEV_NAME,msgTypes[2], msgInfo[3], "Parameters");
    }
}