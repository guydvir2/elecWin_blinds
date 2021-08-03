#include <Arduino.h>

#define DEV_NAME "WEMOS_mini"
#define JSON_SIZE_IOT 400
#define JSON_SIZE_SKETCH 200
#define JSON_SERIAL_SIZE 300
#define VER "ESP8266_V1.0"

#include "myIOT_settings.h"
#include "win_param.h"

const char *winStates[] = {"Error", "up", "down", "off"};
const char *serialKW[] = {"from", "act", "info", "error"};
const char *serialCMD[] = {"status", "reset_MCU", "query", "boot_p", "Boot", "error", "ping"};

unsigned long lastAliveping = 0;
void sendMSG(char *msg, char *addinfo)
{
        StaticJsonDocument<JSON_SERIAL_SIZE> doc;

        doc[serialKW[0]] = DEV_NAME;
        doc[serialKW[1]] = msg;
        if (addinfo == NULL)
        {
                doc[serialKW[2]] = "none";
        }
        else
        {
                doc[serialKW[2]] = addinfo;
        }

        serializeJson(doc, Serial); /* Sending MSG over serial to other MCU */
}
void send_boot_parameters()
{
        uint8_t offset_HRS = 2;
        unsigned long t = time(nullptr) + offset_HRS * 3600;
        StaticJsonDocument<JSON_SERIAL_SIZE> doc;

        doc["from"] = DEV_NAME;
        doc["act"] = serialCMD[3];

        /* Following will update from flash */
        doc["err_p"] = err_protect;
        doc["dub_sw"] = doubleSW;

        doc["t_out"] = useAutoOff;
        doc["t_out_d"] = autoOff_time;
        doc["boot_t"] = t;
        doc["del_off"] = del_off;
        doc["del_loop"] = del_loop;
        doc["btype_2"] = btype_2;
        doc["send_interval_minutes"] = send_interval_minutes;

        serializeJson(doc, Serial);
}
void checkAlive()
{
        static bool notifyAlert = false;

        if (millis() - lastAliveping > send_interval_minutes * 1000 * 60UL + 200)
        {
                if (!notifyAlert)
                {
                        iot.pub_log("[MCU]: not Alive");
                        notifyAlert = true;
                }
                else
                {
                        notifyAlert = false;
                }
        }
}
void Serial_CB(JsonDocument &_doc)
{
        char outmsg[100];
        const char *FROM = _doc[serialKW[0]];
        const char *ACT = _doc[serialKW[1]];
        const char *INFO = _doc[serialKW[2]];

        if (strcmp(ACT, winStates[1]) == 0 || strcmp(ACT, winStates[2]) == 0 || strcmp(ACT, winStates[3]) == 0)
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
        else if (strcmp(ACT, serialCMD[2]) == 0)
        {
                sprintf(outmsg, "[%s]: %s", "Query", INFO);
                iot.pub_msg(outmsg);
        }
        else if (strcmp(ACT, serialCMD[3]) == 0)
        {
                send_boot_parameters();
        }
        else if (strcmp(ACT, serialCMD[4]) == 0)
        {
                const char *FROM = _doc[serialKW[0]];
                sprintf(outmsg, "[%s]: << Power On Boot >>", FROM);
                iot.pub_log(outmsg);
        }
        else if (strcmp(ACT, serialCMD[0]) == 0)
        {
                sprintf(outmsg, "[%s]: Window [%s]", "Status", INFO);
                iot.pub_msg(outmsg);
        }
        else if (strcmp(ACT, serialCMD[5]) == 0)
        {
                sprintf(outmsg, "[%s]: [%s]; from[%s]", "Error", INFO, FROM);
                iot.pub_msg(outmsg);
        }
        else if (strcmp(INFO, serialCMD[6]) == 0)
        {
                lastAliveping = millis();
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
        checkAlive();
        // delay(50);
}