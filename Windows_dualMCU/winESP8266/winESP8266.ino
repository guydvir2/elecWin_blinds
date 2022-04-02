#include <myIOT2.h>
#include "defs.h"
#include "SerialComm.h"
#include "win_param.h"
#include "myIOT_settings.h"

myIOT2 iot;
bool pingOK = false;
unsigned long last_success_ping_clock = 0;

void Serial_CB(JsonDocument &_doc)
{
        char outmsg[150];
        const char *FROM = _doc[msgKW[0]];
        const char *TYPE = _doc[msgKW[1]];
        const char *INFO = _doc[msgKW[2]];
        const char *INFO2 = _doc[msgKW[3]];

        if (strcmp(TYPE, msgTypes[1]) == 0) /* Getting Info */
        {
                if (strcmp(INFO, msgInfo[0]) == 0) /* status */
                {
                        sprintf(outmsg, "[%s]: %s", INFO, INFO2);
                        iot.pub_msg(outmsg);
                }
                else if (strcmp(INFO, msgInfo[1]) == 0) /* Query */
                {
                        sprintf(outmsg, "[%s]: %s", INFO, INFO2);
                        iot.pub_msg(outmsg);
                }
                else if (strcmp(INFO, msgInfo[2]) == 0) /* boot_p */
                {
                        send_boot_parameters();
                }
                else if (strcmp(INFO, msgInfo[3]) == 0) /* Boot */
                {
                        sprintf(outmsg, "[%s]: << Power On Boot >>", FROM);
                        iot.pub_log(outmsg);
                }
                else if (strcmp(INFO, msgInfo[7]) == 0) /* Ping */
                {
                        last_success_ping_clock = millis();
                        pingOK = true;
                }
                else if (strcmp(INFO, msgAct[6]) == 0 && Lockdown) /* LOCKDONW_ON */
                {
                        sprintf(outmsg, "Locdown: [%s] is locked", FROM);
                        iot.pub_log(outmsg);
                }
                else if (strcmp(INFO, msgAct[7]) == 0 && Lockdown) /* LOCKDONW_OFF */
                {
                        sprintf(outmsg, "Locdown: [%s] is released", FROM);
                        iot.pub_log(outmsg);
                }
        }
        else if (strcmp(TYPE, msgTypes[0]) == 0) /*  Actions */
        {
                if (strcmp(INFO, msgAct[WIN_UP]) == 0 || strcmp(INFO, msgAct[WIN_DOWN]) == 0 || strcmp(INFO, msgAct[WIN_STOP]) == 0 || strcmp(INFO, msgAct[5]) == 0)
                {
                        sprintf(outmsg, "[%s]: Window switched [%s]", INFO2, INFO);
                        iot.pub_msg(outmsg);
                        iot.pub_state((char *)INFO, 0); // Publishing State!
                }
        }
        else if (strcmp(TYPE, msgTypes[2]) == 0) /* Errors  */
        {
                sprintf(outmsg, "[%s]: [%s] [%s] [%s]", TYPE, FROM, INFO, INFO2);
                iot.pub_log(outmsg);
        }
}
void ping_looper(uint8_t loop_period = 10)
{
        static unsigned long last_ping_clock = 0;
        static bool err_notification = false;
        const uint8_t extra_time_to_err = 1;
        const int time_constant = 1000;

        if (pingOK == false && err_notification == false && (last_ping_clock != 0 && millis() - last_ping_clock > 1000)) /* Notify reach failure*/
        {
                err_notification = true;
                iot.pub_log("[Ping]: [Fail] reaching MCU");
        }
        else if (pingOK && err_notification) /* Notify restore Ping*/
        {
                err_notification = false;
                iot.pub_log("[Ping]: [Restored] reaching MCU");
        }

        if (millis() - last_success_ping_clock > loop_period * time_constant + 1000 * extra_time_to_err && pingOK) /* fail getting ping back */
        {
                pingOK = false; /* Change state to fail */
        }
        else if (millis() - last_ping_clock > loop_period * time_constant || last_ping_clock == 0) /* init sending ping due to time */
        {
                last_ping_clock = millis();
                sendMSG(msgTypes[1], msgInfo[7]);
        }
}
void setup()
{
        readFlash_parameters();
        startIOTservices();
        Serial.begin(9600); /* Serial is defined not using IOT - else it spits all debug msgs */
}
void loop()
{
        iot.looper();
        lockdown_looper();
        ping_looper(30);
        readSerial();
}
