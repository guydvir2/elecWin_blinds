#include <Arduino.h>
#include <ArduinoJson.h>

const char *winStates[] = {"Error", "up", "down", "off"};
const char *msgKW[] = {"from", "type", "i", "i_ext"};
const char *msgTypes[] = {"act", "info", "error"};
const char *msgAct[] = {winStates[0], winStates[1], winStates[2], winStates[3], "reset_MCU", "Auto-Off", "lockdown_on", "lockdown_off"};
const char *msgInfo[] = {"status", "query", "boot_p", "Boot", "error", "button", "MQTT", "ping", "Ext_button"};
const char *msgErrs[] = {"Comm", "Parameters", "Boot", "unKnown-error"};


extern void Serial_CB(JsonDocument &_doc);
extern myIOT2 iot;

void sendMSG(const char *msgtype, const char *addinfo, const char *info2 = "0")
{
    StaticJsonDocument<JSON_SERIAL_SIZE> doc;

    doc[msgKW[0]] = DEV_NAME;
    doc[msgKW[1]] = msgtype;
    doc[msgKW[2]] = addinfo;
    doc[msgKW[3]] = info2;

    serializeJson(doc, Serial); /* Sending MSG over serial to other MCU */
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