#include <myIOT2.h>
#include "variables.h"
#include "myIOT_settings.h"
#include "win_param.h"
#include <Arduino.h>

bool services_chk()
{
        bool ntp = iot.NTP_OK;
        bool wifi = WiFi.isConnected();
        bool mqtt = iot.mqttClient.connected();

        if (!(wifi && mqtt && ntp))
        {
                char a[50];
                sprintf(a, "Services error: WiFi[%s] MQTT[%s] NTP[%s]", wifi ? "OK" : "FAIL", mqtt ? "OK" : "FAIL", ntp ? "OK" : "FAIL");
                iot.pub_log(a);
                return 0;
        }
        else
        {
                return 1;
        }
}
void check_bootclockLOG()
{
        char a[100];
        char clk[20];
        char dat[20];
        const byte MIN_HRS_BETWEEN_RESET = 24;                                                                           /* 1 day between 2 consq. resets */
        const byte MAX_HRS_ALL_RESETS = 24 * 7;                                                                          /* 1 week between all resets archived ( default is 3 stores ) */
        unsigned long first_to_last = iot.get_bootclockLOG(0) / 3600 - iot.get_bootclockLOG(iot.bootlog_len - 1) / 3600; // hours
        unsigned long first_to_second = iot.get_bootclockLOG(0) / 3600 - iot.get_bootclockLOG(1) / 3600;                 //hours
        unsigned int rem_a = iot.get_bootclockLOG(0) % 3600 - iot.get_bootclockLOG(iot.bootlog_len - 1) % 3600;
        unsigned int rem_b = iot.get_bootclockLOG(0) % 3600 - iot.get_bootclockLOG(1) % 3600;

        if (first_to_last < MAX_HRS_ALL_RESETS)
        {
                iot.convert_epoch2clock(first_to_last * 3600 + rem_a, 0, clk, dat);
                sprintf(a, "Reset Errors: got [%d] resets in [%s]. Limit is [%d]hrs", iot.bootlog_len, clk, MAX_HRS_ALL_RESETS);
                iot.pub_log(a);
        }
        if (first_to_second < MIN_HRS_BETWEEN_RESET)
        {
                iot.convert_epoch2clock(first_to_second * 3600 + rem_b, 0, clk, dat);
                sprintf(a, "Reset Errors: got [%d] resets in [%s]. Limit is [%d]hrs", 2, clk, MIN_HRS_BETWEEN_RESET);
                iot.pub_log(a);
        }
        Serial.print("max: ");
        Serial.println(first_to_last);
        Serial.print("min: ");
        Serial.println(first_to_second);
}
void check_reboot_reason()
{
        static bool checkAgain = true;
        if (checkAgain)
        {
                if (iot.mqtt_detect_reset != 2)
                {
                        char a[30];
                        checkAgain = false;
                        if (iot.mqtt_detect_reset == 0)
                        {
                                sprintf(a, "Boot Type: [%s]", "Boot");
                        }
                        else if (iot.mqtt_detect_reset == 1)
                        {
                                sprintf(a, "Boot Type: [%s]", "Quick-Reset");
                        }
                        iot.pub_log(a);
                }
        }
}

void startGPIOs()
{
        pinMode(inputUpPin, INPUT_PULLUP);
        pinMode(inputDownPin, INPUT_PULLUP);
        if (useExtInput)
        {
                pinMode(inputUpExtPin, INPUT_PULLUP);
                pinMode(inputDownExtPin, INPUT_PULLUP);
        }

        pinMode(outputUpPin, OUTPUT);
        pinMode(outputDownPin, OUTPUT);

        allOff();
}
void allOff()
{
        digitalWrite(outputUpPin, !RelayOn);
        digitalWrite(outputDownPin, !RelayOn);
        inputUp_lastState = digitalRead(inputUpPin);
        inputDown_lastState = digitalRead(inputDownPin);
        if (useExtInput)
        {
                inputUpExt_lastState = digitalRead(inputUpExtPin);
                inputDownExt_lastState = digitalRead(inputDownExtPin);
        }
}
void switchIt(char *type, char *dir)
{
        char mqttmsg[50];
        bool states[2];

        if (strcmp(dir, "up") == 0)
        {
                states[0] = RelayOn;
                states[1] = !RelayOn;
        }
        else if (strcmp(dir, "down") == 0)
        {
                states[0] = !RelayOn;
                states[1] = RelayOn;
        }
        else if (strcmp(dir, "off") == 0)
        {
                states[0] = !RelayOn;
                states[1] = !RelayOn;
        }

        bool Up_read = digitalRead(outputUpPin);
        bool Down_read = digitalRead(outputDownPin);

        // Case that both realys need to change state ( Up --> Down or Down --> Up )
        if (Up_read != states[0] && Down_read != states[1])
        {
                allOff();
                delay(deBounceInt * 2);
                digitalWrite(outputUpPin, states[0]);
                digitalWrite(outputDownPin, states[1]);
        }
        // Case that one relay changes from/to off --> on
        else if (Up_read != states[0] || Down_read != states[1])
        {
                digitalWrite(outputUpPin, states[0]);
                digitalWrite(outputDownPin, states[1]);
        }
        iot.pub_state(dir);
        sprintf(mqttmsg, "%s: Switched [%s]", type, dir);
        iot.pub_msg(mqttmsg);

        if (useAutoRelayOFF)
        {
                if (digitalRead(outputDownPin) == RelayOn || digitalRead(outputUpPin) == RelayOn)
                {
                        autoOff_clock = millis();
                }
                else
                {
                        // when switched off, cancel timeout
                        autoOff_clock = 0;
                }
        }
}
void checkSwitch_looper(const int &pin, char *dir, bool &lastState, char *type = "Button")
{
        if (digitalRead(pin) != lastState)
        {
                delay(deBounceInt);
                if (digitalRead(pin) != lastState)
                {
                        if (digitalRead(pin) == SwitchOn)
                        {
                                switchIt(type, dir);
                                lastState = digitalRead(pin);
                        }
                        else if (digitalRead(pin) == !SwitchOn)
                        {
                                switchIt(type, "off");
                                lastState = digitalRead(pin);
                        }
                }
                else if (USE_BOUNCE_DEBUG)
                { // for debug only
                        char tMsg[100];
                        sprintf(tMsg, "[%s] Bounce: cRead[%d] lRead[%d]", dir, digitalRead(pin), lastState);
                        iot.pub_log(tMsg);
                }
        }
}
void checkTimeout_AutoRelay_Off(int timeout_off)
{
        if (autoOff_clock != 0 && millis() - autoOff_clock > timeout_off * 1000)
        {
                switchIt("timeout", "off");
                autoOff_clock = 0;
        }
}
void verifyNotHazardState()
{
        if (digitalRead(outputUpPin) == RelayOn && digitalRead(outputDownPin) == RelayOn)
        {
                switchIt("Button", "off");
                iot.sendReset("HazradState");
        }
}

void setup()
{
        startRead_parameters();
        startGPIOs();
        startIOTservices();
        endRead_parameters();

        services_chk();
        check_bootclockLOG();
}
void loop()
{
        iot.looper();
        verifyNotHazardState(); // both up and down are ---> OFF

        checkSwitch_looper(inputUpPin, "up", inputUp_lastState, "inButton");
        checkSwitch_looper(inputDownPin, "down", inputDown_lastState, "inButton");
        if (useExtInput)
        {
                checkSwitch_looper(inputUpExtPin, "up", inputUpExt_lastState, "extButton");
                checkSwitch_looper(inputDownExtPin, "down", inputDownExt_lastState, "extButton");
        }
        if (useAutoRelayOFF)
        {
                checkTimeout_AutoRelay_Off(AutoRelayOff_timeout);
        }
        check_reboot_reason();
        delay(100);
}
