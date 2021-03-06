#include <myIOT2.h>
#include "win_param.h"

myIOT2 iot;

extern char *sketch_paramfile;
extern void switchIt(char *type, uint8_t dir);

void addiotnalMQTT(char *incoming_msg)
{
    char state[5];
    char state2[5];
    char msg[100];
    char msg2[100];

    if (strcmp(incoming_msg, "status") == 0)
    {
        // relays state
        if (digitalRead(outputUpPin) == RelayOn && digitalRead(outputDownPin) == RelayOn)
        {
            sprintf(state, "invalid Relay State");
        }
        else if (digitalRead(outputUpPin) == !RelayOn && digitalRead(outputDownPin) == RelayOn)
        {
            sprintf(state, "DOWN");
        }
        else if (digitalRead(outputUpPin) == RelayOn && digitalRead(outputDownPin) == !RelayOn)
        {
            sprintf(state, "UP");
        }
        else
        {
            sprintf(state, "OFF");
        }

        // switch state
        //         uint8_t readSW = windowSwitch.getValue();
        // if (inputUp_lastState == !RelayOn && inputDown_lastState == !RelayOn)
        // {
        //     sprintf(state2, "OFF");
        // }
        // else if (inputUp_lastState == RelayOn && inputDown_lastState == !RelayOn)
        // {
        //     sprintf(state2, "UP");
        // }
        // else if (inputUp_lastState == !RelayOn && inputDown_lastState == RelayOn)
        // {
        //     sprintf(state2, "DOWN");
        // }
        // if(digitalRead)
        // sprintf(msg, "Status: Relay:[%s], Switch:[%s]", state, state2);
        sprintf(msg, "Status: Relay:[%s]", state);

        iot.pub_msg(msg);
    }
    else if (strcmp(incoming_msg, "up") == 0)
    {
        switchIt("MQTT", 1);
    }
    else if (strcmp(incoming_msg, "down") == 0)
    {
        switchIt("MQTT", 2);
    }
    else if (strcmp(incoming_msg, "off") == 0)
    {
        switchIt("MQTT", 3);
    }
    else if (strcmp(incoming_msg, "ver2") == 0)
    {
        sprintf(msg, "ver2:[%s], AutoOFF[%d], AutoOFFduration[%d sec]", VER, useAutoRelayOFF, AutoRelayOff_timeout);
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
        sprintf(msg, "GPIO pins: inputUP[%d], inputDown[%d], outputUP[%d], outputDown[%d], useExtPins[%d], extUp[%d], extDown[%d]",
                inputUpPin, inputDownPin, outputUpPin, outputDownPin, useExtInput, inputUpExtPin, inputDownExtPin);
        iot.pub_msg(msg);
    }
}
void startIOTservices()
{
    iot.useSerial = paramJSON["useSerial"];
    iot.useWDT = paramJSON["useWDT"];
    iot.useOTA = paramJSON["useOTA"];
    iot.useResetKeeper = paramJSON["useResetKeeper"];
    iot.useDebug = paramJSON["useDebugLog"];
    iot.debug_level = paramJSON["debug_level"]; //All operations are monitored
    iot.useBootClockLog = true;
    iot.useNetworkReset = paramJSON["useNetworkReset"];
    iot.noNetwork_reset = paramJSON["noNetwork_reset"];
    strcpy(iot.deviceTopic, paramJSON["deviceTopic"]);
    strcpy(iot.prefixTopic, paramJSON["prefixTopic"]);
    strcpy(iot.addGroupTopic, paramJSON["groupTopic"]);
    iot.start_services(addiotnalMQTT);
}
