#include <ArduinoJson.h>

#if DEBUG_MODE
#include <SoftwareSerial.h>
SoftwareSerial mySerial(9, 8); // RX, TX
#endif

const char *winStates[] = {"off", "up", "down", "Error"};
const char *msgKW[] = {"from", "type", "i", "i_ext"};
const char *msgTypes[] = {"act", "info", "error"};
const char *msgAct[] = {winStates[0], winStates[1], winStates[2], winStates[3], "reset_MCU", "Auto-Off", "lockdown_on", "lockdown_off"};
const char *msgInfo[] = {"status", "query", "boot_p", "Boot", "error", "button", "MQTT", "ping", "Ext_button"};
const char *msgErrs[] = {"Comm", "Parameters", "Boot", "unKnown-error"};

// ~~~~~~~~~ Serial Communication ~~~~~~~~

extern void Serial_CB(JsonDocument &_doc);

void _constructMSG(JsonDocument &doc, const char *KW1, const char *KW2, const char *KW3)
{
    doc[msgKW[0]] = DEV_NAME;
    doc[msgKW[1]] = KW1;
    doc[msgKW[2]] = KW2;
    doc[msgKW[3]] = KW3;
}
void sendMSG(const char *msgtype, const char *ext1, const char *ext2 = "0")
{
    StaticJsonDocument<JSON_SERIAL_SIZE> doc;

    _constructMSG(doc, msgtype, ext1, ext2);
    serializeJson(doc, Serial);
#if DEBUG_MODE
    Serial.print("\nSent: ");
    serializeJson(doc, Serial);
#endif
}
void readSerial()
{
    if (Serial.available() > 0)
    {
        StaticJsonDocument<JSON_SERIAL_SIZE> doc;
        DeserializationError error = deserializeJson(doc, Serial);

        if (!error)
        {
#if DEBUG_MODE
            Serial.print("\nGot msg: ");
            serializeJson(doc, Serial);
            Serial.flush();
#endif
            Serial_CB(doc); /* send message to Callback */
        }
        else
        {
            while (Serial.available() > 0) /* Clear Buffer in case of error */
            {
                Serial.read();
            }
            sendMSG(msgTypes[2], msgErrs[0]); /* Communication Error */
        }
    }
}
