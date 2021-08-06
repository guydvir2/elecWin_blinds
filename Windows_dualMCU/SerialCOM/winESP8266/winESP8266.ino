#include <Arduino.h>

#define DEV_NAME "WEMOS_mini"
#define JSON_SIZE_IOT 400
#define JSON_SIZE_SKETCH 300
#define JSON_SERIAL_SIZE 450
#define VER "ESP8266_V1.0"

#include "myIOT_settings.h"
#include "win_param.h"

char *winStates[] = {"Error", "up", "down", "off"};
char *msgKW[] = {"from", "type", "info", "info2"};
char *msgTypes[] = {"act", "info", "error"};
char *msgAct[] = {winStates[1], winStates[2], winStates[3], "reset_MCU", "Auto-Off"};
char *msgInfo[] = {"status", "query", "boot_p", "Boot", "error", "ping", "button", "MQTT"};
char *msgErrs[] = {"Comm", "Parameters", "Boot", "unKnown-error"};

unsigned long lastAliveping = 0;

void sendMSG(char *msgtype, char *addinfo, char *info2)
{
        StaticJsonDocument<JSON_SERIAL_SIZE> doc;

        doc[msgKW[0]] = DEV_NAME;
        doc[msgKW[1]] = msgtype;
        doc[msgKW[2]] = addinfo;
        doc[msgKW[3]] = info2;

        char testouput[200];
        serializeJson(doc, Serial); /* Sending MSG over serial to other MCU */
        // serializeJson(doc, testouput);
        // iot.pub_msg(testouput);
}
void send_boot_parameters()
{
        uint8_t offset_HRS = 2;
        unsigned long t = time(nullptr); //+ offset_HRS * 3600;
        StaticJsonDocument<JSON_SERIAL_SIZE> doc;

        doc[msgKW[0]] = DEV_NAME;
        doc[msgKW[1]] = msgTypes[1];
        doc[msgKW[2]] = msgInfo[2];

        /* Following will update from flash */
        doc["err_p"] = err_protect;
        doc["dub_sw"] = doubleSW;

        doc["t_out"] = useAutoOff;
        doc["t_out_d"] = autoOff_time;
        doc["boot_t"] = t;
        doc["del_off"] = del_off;
        doc["del_loop"] = del_loop;
        doc["btype_2"] = btype_2;
        doc["Alive_int"] = Alive_int;

        serializeJson(doc, Serial);
}
void Serial_CB(JsonDocument &_doc)
{
        char outmsg[100];
        const char *FROM = _doc[msgKW[0]];
        const char *TYPE = _doc[msgKW[1]];
        const char *INFO = _doc[msgKW[2]];
        const char *INFO2 = _doc[msgKW[3]];

        if (strcmp(TYPE, msgTypes[1]) == 0) /* Getting Info */
        {
                if (strcmp(INFO, msgAct[0]) == 0 || strcmp(INFO, msgAct[1]) == 0 || strcmp(INFO, msgAct[2]) == 0)
                {
                        sprintf(outmsg, "[%s]: Window switched [%s]", INFO2, INFO);
                        iot.pub_msg(outmsg);
                }
                else if (strcmp(INFO, msgInfo[2]) == 0) /* boot_p */
                {
                        send_boot_parameters();
                }
                else if (strcmp(INFO, msgInfo[1]) == 0) /* Query */
                {
                        sprintf(outmsg, "[%s]: %s", INFO, INFO2);
                        iot.pub_msg(outmsg);
                }
                else if (strcmp(INFO, msgInfo[3]) == 0) /* Boot */
                {
                        sprintf(outmsg, "[%s]: << Power On Boot >>", FROM);
                        iot.pub_log(outmsg);
                }
        }
        else if (strcmp(TYPE, msgTypes[0]) == 0) /*  Actions */
        {
                if (strcmp(INFO, msgAct[0]) == 0 || strcmp(INFO, msgAct[1]) == 0 || strcmp(INFO, msgAct[2]) == 0 || strcmp(INFO, msgAct[4]) == 0)
                {
                        sprintf(outmsg, "[%s]: Window switched [%s]", INFO2, INFO);
                        iot.pub_msg(outmsg);
                }
        }
        else if (strcmp(TYPE, msgTypes[2]) == 0) /* Errors  */
        {
                sprintf(outmsg, "[%s]: [%s] [%s] [%s]", TYPE, FROM, INFO, INFO2);
                iot.pub_msg(outmsg);

                // char testouput[100];
                // serializeJson(_doc, testouput);
                // iot.pub_msg(testouput);
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
        // checkAlive();
        // delay(50);
}
