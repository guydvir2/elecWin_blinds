#include <myIOT2.h>
#define DEV_NAME "WEMOS_mini"
#define JSON_SIZE_IOT 400
#define JSON_SIZE_SKETCH 300
#define JSON_SERIAL_SIZE 250
#define VER "ESP8266_V1.2b"

myIOT2 iot;

enum sys_states : const uint8_t
{
        WIN_ERR,
        WIN_UP,
        WIN_DOWN,
        WIN_STOP,
};

const char *winStates[] = {"Error", "up", "down", "off"};
const char *msgKW[] = {"from", "type", "i", "i_ext"};
const char *msgTypes[] = {"act", "info", "error"};
const char *msgAct[] = {winStates[0], winStates[1], winStates[2], winStates[3], "reset_MCU", "Auto-Off", "lockdown_on", "lockdown_off"};
const char *msgInfo[] = {"status", "query", "boot_p", "Boot", "error", "button", "MQTT", "ping"};
const char *msgErrs[] = {"Comm", "Parameters", "Boot", "unKnown-error"};

#include "win_param.h"
#include "myIOT_settings.h"

bool pingOK = false;
unsigned long last_success_ping_clock = 0;

void ping_looper(uint8_t loop_period = 10)
{
        static unsigned long last_ping_clock = 0;
        static bool err_notification = false;
        const uint8_t extra_time_to_err = 1;

        if (pingOK == false && err_notification == false && (last_ping_clock != 0 && millis() - last_ping_clock > 1000)) /* Notify reach failure*/
        {
                err_notification = true;
                iot.pub_log("[Ping]: [Fail] reaching MCU");
        }
        else if (pingOK && err_notification) /* Notify restore Ping*/
        {
                err_notification = false;
        }

        if (millis() - last_success_ping_clock > loop_period * 60000UL + 1000 * extra_time_to_err && pingOK) /* Change state to fail */
        {
                pingOK = false;
        }
        else if (millis() - last_ping_clock > loop_period * 60000UL || last_ping_clock == 0) /* init sending ping due to time */
        {
                last_ping_clock = millis();
                sendMSG(msgTypes[1], msgInfo[7]);
        }
}
void sendMSG(const char *msgtype, const char *addinfo, const char *info2)
{
        StaticJsonDocument<JSON_SERIAL_SIZE> doc;

        doc[msgKW[0]] = DEV_NAME;
        doc[msgKW[1]] = msgtype;
        doc[msgKW[2]] = addinfo;
        doc[msgKW[3]] = info2;

        serializeJson(doc, Serial); /* Sending MSG over serial to other MCU */
}
void send_boot_parameters()
{
        StaticJsonDocument<JSON_SERIAL_SIZE> doc;

        doc[msgKW[0]] = DEV_NAME;
        doc[msgKW[1]] = msgTypes[1];
        doc[msgKW[2]] = msgInfo[2];

        /* Following parameters will update from flash */
        doc["err_p"] = err_protect;
        doc["dub_sw"] = doubleSW;
        doc["t_out"] = useAutoOff;
        doc["t_out_d"] = autoOff_time;
        doc["boot_t"] = (long)iot.now();
        doc["btype_2"] = btype_2;
        doc["Lockdown"] = Lockdown;

        serializeJson(doc, Serial);
}
void Serial_CB(JsonDocument &_doc)
{
        char outmsg[150];
        const char *FROM = _doc[msgKW[0]];
        const char *TYPE = _doc[msgKW[1]];
        const char *INFO = _doc[msgKW[2]];
        const char *INFO2 = _doc[msgKW[3]];

        if (strcmp(TYPE, msgTypes[1]) == 0)      /* Getting Info */
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
                if (strcmp(INFO, msgAct[WIN_UP]) == 0 || strcmp(INFO, msgAct[WIN_DOWN]) == 0 || strcmp(INFO, msgAct[WIN_STOP]) == 0 || strcmp(INFO, msgAct[4]) == 0)
                {
                        sprintf(outmsg, "[%s]: Window switched [%s]", INFO2, INFO);
                        iot.pub_msg(outmsg);
                }
        }
        else if (strcmp(TYPE, msgTypes[2]) == 0) /* Errors  */
        {
                sprintf(outmsg, "[%s]: [%s] [%s] [%s]", TYPE, FROM, INFO, INFO2);
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
        lockdown_looper();
        readSerial();
        ping_looper(30);
}
