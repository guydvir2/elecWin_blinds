#include <myIOT2.h>

myIOT2 iot;
extern char *sketch_paramfile;
extern byte get_relayState();
extern void makeSwitch(byte state);
extern StaticJsonDocument<JSON_SIZE_IOT> paramJSON;
extern StaticJsonDocument<JSON_SIZE_SKETCH> sketchJSON;

void addiotnalMQTT(char *incoming_msg)
{
    char state[5];
    char msg[100];
    char msg2[100];

    if (strcmp(incoming_msg, "status") == 0)
    {
        // relays state
        char a[5];
        byte relcheck = get_relayState();
        switch (relcheck)
        {
        case WIN_STOP:
            strcpy(a, "Off");
            break;
        case WIN_UP:
            strcpy(a, "Up");
            break;
        case WIN_DOWN:
            strcpy(a, "Down");
            break;
        default:
            strcpy(a, "Error");
            break;
        }
        sprintf(msg, "Status: Window\'s relay state [%s]", a);
        iot.pub_msg(msg);
    }
    else if (strcmp(incoming_msg, "up") == 0)
    {
        makeSwitch(WIN_UP);
        iot.pub_msg("MQTT: Switch [up]");
    }
    else if (strcmp(incoming_msg, "down") == 0)
    {
        makeSwitch(WIN_DOWN);
        iot.pub_msg("MQTT: Switch [down]");
    }
    else if (strcmp(incoming_msg, "off") == 0)
    {
        makeSwitch(WIN_STOP);
        iot.pub_msg("MQTT: Switch [off]");
    }
    else if (strcmp(incoming_msg, "ver2") == 0)
    {
        sprintf(msg, "ver2:[%s]", VER);
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
        sprintf(msg, "Help: Commands #3 - [up, down, off, gpios, show_flash_param]");
        iot.pub_msg(msg);
    }
    else if (strcmp(incoming_msg, "gpios") == 0)
    {
        sprintf(msg, "GPIO pins: outputUP[%d], outputDown[%d], relayUPindic[%d], relayDownindic[%d]", outputUpPin, outputDownPin, relayUpPin, relayDownPin);
        iot.pub_msg(msg);
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
