#include <myIOT2.h>

myIOT2 iot;
extern char *sketch_paramfile;
extern void sendMSG(char *msg, char *addinfo = NULL);
extern bool useAutoOff;
extern int autoOff_time;

extern StaticJsonDocument<JSON_SIZE_IOT> paramJSON;
extern StaticJsonDocument<JSON_SIZE_SKETCH> sketchJSON;

void addiotnalMQTT(char *incoming_msg)
{
    char msg[100];

    if (strcmp(incoming_msg, "status") == 0)
    {
        sendMSG("status");
    }
    else if (strcmp(incoming_msg, "up") == 0 || strcmp(incoming_msg, "down") == 0 || strcmp(incoming_msg, "off") == 0)
    {
        sendMSG(incoming_msg, "MQTT");
    }
    else if (strcmp(incoming_msg, "ver2") == 0)
    {
        sprintf(msg, "ver2:[%s], [DualMCU], [Serial-Comm], AutoOff[%d], AutoOff_time[%d]", VER, useAutoOff, autoOff_time);
        iot.pub_msg(msg);
    }
    else if (strcmp(incoming_msg, "show_flash_param") == 0)
    {
        char temp[300];
        char temp3[350];
        char *a[] = {iot.myIOT_paramfile, sketch_paramfile};
        iot.pub_debug("~~~Start~~~");
        for (int e = 0; e < sizeof(a) / sizeof(a[0]); e++)
        {
            strcpy(temp, iot.export_fPars(a[e], paramJSON));
            sprintf(temp3, "%s: %s", a[e], temp);
            iot.pub_debug(temp3);
            paramJSON.clear();
        }
        iot.pub_debug("~~~End~~~");
    }
    else if (strcmp(incoming_msg, "help2") == 0)
    {
        sprintf(msg, "Help: Commands #3 - [up, down, off, query, reset_MCU, show_flash_param, help2]");
        iot.pub_msg(msg);
    }
    else if (strcmp(incoming_msg, "query") == 0)
    {
        sendMSG("query");
    }
    else if (strcmp(incoming_msg, "reset_MCU") == 0)
    {
        sprintf(msg, "Reset: sent to MCU");
        iot.pub_msg(msg);
        sendMSG("reset_MCU");
    }
}
void startIOTservices()
{
    iot.useSerial = paramJSON["useSerial"];
    iot.useWDT = paramJSON["useWDT"];
    iot.useOTA = paramJSON["useOTA"];
    iot.useResetKeeper = paramJSON["useResetKeeper"];
    iot.resetFailNTP = paramJSON["useFailNTP"];
    iot.useDebug = paramJSON["useDebugLog"];
    iot.debug_level = paramJSON["debug_level"];
    iot.useBootClockLog = paramJSON["useBootClockLog"];
    strcpy(iot.deviceTopic, paramJSON["deviceTopic"]);
    strcpy(iot.prefixTopic, paramJSON["prefixTopic"]);
    strcpy(iot.addGroupTopic, paramJSON["groupTopic"]);
    iot.start_services(addiotnalMQTT);
}

// ~~~~~~~ Additional Services ~~~~~~~~
bool services_chk()
{
    if (!(WiFi.isConnected() && iot.mqttClient.connected() && iot.NTP_OK))
    {
        char a[50];
        sprintf(a, "Services error: WiFi[%s] MQTT[%s] NTP[%s]",
                WiFi.isConnected() ? "OK" : "FAIL", iot.mqttClient.connected() ? "OK" : "FAIL", iot.NTP_OK ? "OK" : "FAIL");
        iot.pub_log(a);
        return 0;
    }
    else
    {
        return 1;
    }
}
void check_bootclockLOG()
{
    char a[100];
    char clk[20];
    char dat[20];
    const byte MIN_HRS_BETWEEN_RESET = 24;                                                                           /* 1 day between 2 consq. resets */
    const byte MAX_HRS_ALL_RESETS = 24 * 7;                                                                          /* 1 week between all resets archived ( default is 3 stores ) */
    unsigned long first_to_last = iot.get_bootclockLOG(0) / 3600 - iot.get_bootclockLOG(iot.bootlog_len - 1) / 3600; // hours
    unsigned long first_to_second = iot.get_bootclockLOG(0) / 3600 - iot.get_bootclockLOG(1) / 3600;                 //hours
    unsigned int rem_a = iot.get_bootclockLOG(0) % 3600 - iot.get_bootclockLOG(iot.bootlog_len - 1) % 3600;
    unsigned int rem_b = iot.get_bootclockLOG(0) % 3600 - iot.get_bootclockLOG(1) % 3600;

    if (first_to_last < MAX_HRS_ALL_RESETS)
    {
        iot.convert_epoch2clock(first_to_last * 3600 + rem_a, 0, clk, dat);
        sprintf(a, "Reset Errors: got [%d] resets in [%s]. Limit is [%d]hrs", iot.bootlog_len, clk, MAX_HRS_ALL_RESETS);
        iot.pub_log(a);
    }
    if (first_to_second < MIN_HRS_BETWEEN_RESET)
    {
        iot.convert_epoch2clock(first_to_second * 3600 + rem_b, 0, clk, dat);
        sprintf(a, "Reset Errors: got [%d] resets in [%s]. Limit is [%d]hrs", 2, clk, MIN_HRS_BETWEEN_RESET);
        iot.pub_log(a);
    }
}
void check_reboot_reason()
{
    static bool checkAgain = true;
    if (checkAgain)
    {
        if (iot.mqtt_detect_reset != 2)
        {
            char a[30];
            checkAgain = false;
            if (iot.mqtt_detect_reset == 0)
            {
                sprintf(a, "Boot Type: [%s]", "Boot");
            }
            else if (iot.mqtt_detect_reset == 1)
            {
                sprintf(a, "Boot Type: [%s]", "Quick-Reset");
            }
            iot.pub_log(a);
        }
    }
}
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
