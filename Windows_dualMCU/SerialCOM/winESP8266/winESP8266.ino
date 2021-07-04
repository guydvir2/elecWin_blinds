#include <myIOT2.h>
#include <Arduino.h>

#define DEV_NAME "WEMOSmini"
#define JSON_SIZE_IOT 400
#define JSON_SIZE_SKETCH 200
#define JSON_SERIAL_SIZE 300
#define VER "ESP8266_V0.9"

#include "myIOT_settings.h"
#include "win_param.h"

void sendMSG(char *msg, char *addinfo)
{
        StaticJsonDocument<JSON_SERIAL_SIZE> doc;

        doc["from"] = DEV_NAME;
        doc["act"] = msg;
        if (addinfo == NULL)
        {
                doc["info"] = "none";
        }
        else
        {
                doc["info"] = addinfo;
        }

        serializeJson(doc, Serial); /* Sending MSG over serial to other MCU */
}
void getBOOT_P()
{
        StaticJsonDocument<JSON_SERIAL_SIZE> doc;
        doc["from"] = DEV_NAME;
        doc["act"] = "boot_p";
        doc["err_p"] = err_protect;
        doc["dub_sw"] = doubleSW;
        doc["t_out"] = useAutoOff;
        doc["t_out_d"] = autoOff_time;
        doc["boot_t"] = now();
        doc["del_off"] = del_off;
        doc["del_loop"] = del_loop;
        serializeJson(doc, Serial);
}
void Serial_CB(JsonDocument &_doc)
{
        char outmsg[100];
        const char *FROM = _doc["from"];
        const char *ACT = _doc["act"];
        const char *INFO = _doc["info"];

        if (strcmp(ACT, "up") == 0 || strcmp(ACT, "down") == 0 || strcmp(ACT, "off") == 0)
        {
                sprintf(outmsg, "[%s]: Window [%s]", INFO, ACT);
                iot.pub_msg(outmsg);
                if (strcmp(INFO, "Auto-Off") != 0)
                {
                        char state[10];
                        sprintf(state, "%s", ACT);
                        iot.pub_state(state);
                }
        }
        else if (strcmp(ACT, "query") == 0)
        {
                sprintf(outmsg, "[%s]: %s", "Query", INFO);
                iot.pub_msg(outmsg);
        }
        else if (strcmp(ACT, "Boot") == 0)
        {
                const char *FROM = _doc["from"];
                sprintf(outmsg, "[%s]: << Power On Boot >>", FROM);
                iot.pub_log(outmsg);
        }
        else if (strcmp(ACT, "status") == 0)
        {
                sprintf(outmsg, "[%s]: Window [%s]", "Status", INFO);
                iot.pub_msg(outmsg);
        }
        else if (strcmp(ACT, "error") == 0)
        {
                sprintf(outmsg, "[%s]: [%s]; from[%s]", "Error", INFO, FROM);
                iot.pub_msg(outmsg);
        }
        else if (strcmp(ACT, "boot_p") == 0)
        {
                getBOOT_P();
        }
        else
        {
                sprintf(outmsg, "[%s]: [Unknown]; from[%s]", "Error", FROM);
                iot.pub_msg(outmsg);
        }
}
void readSerial()
{
        if (Serial.available())
        {
                StaticJsonDocument<JSON_SERIAL_SIZE> doc;
                DeserializationError error = deserializeJson(doc, Serial);

                if (!error)
                {
                        Serial_CB(doc);
                }
                else
                {
                        char aa[40];
                        sprintf(aa, "[%s]: [%s]; from[%s]", "Error", "Recv-error", DEV_NAME);
                        iot.pub_msg(aa);
                }
        }
}

void setup()
{
        startRead_parameters();
        startIOTservices();
        endRead_parameters();

        Serial.begin(9600); /* Serial is defined not using IOT - else it spits all debug msgs */
}
void loop()
{
        iot.looper();
        readSerial();
        // delay(50);
}