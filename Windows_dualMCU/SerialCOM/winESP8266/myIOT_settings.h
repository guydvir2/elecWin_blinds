#include <myIOT2.h>

myIOT2 iot;
extern char *sketch_paramfile;
extern void sendMSG(char *msg, char *addinfo = NULL);
extern bool useAutoOff;
extern uint8_t autoOff_time;

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
    iot.useDebug = paramJSON["useDebugLog"];
    iot.debug_level = paramJSON["debug_level"];
    iot.useBootClockLog = paramJSON["useBootClockLog"];
    strcpy(iot.deviceTopic, paramJSON["deviceTopic"]);
    strcpy(iot.prefixTopic, paramJSON["prefixTopic"]);
    strcpy(iot.addGroupTopic, paramJSON["groupTopic"]);
    iot.start_services(addiotnalMQTT);
}
