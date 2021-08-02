#include <Arduino.h>
#include <buttonPresses.h>

// ********** Sketch Services  ***********
#define VER "WinCtrl_8.0"
#define RelayOn LOW

bool useExtInput;
bool useAutoRelayOFF;
uint8_t AutoRelayOff_timeout;
unsigned long autoOff_clock = 0;

//~~~~ Switches IOs~~~~~~
uint8_t inputUpPin;
uint8_t inputDownPin;
uint8_t inputUpExtPin;
uint8_t inputDownExtPin;
uint8_t outputUpPin;
uint8_t outputDownPin;

#include "myIOT_settings.h"
buttonPresses windowSwitch;
buttonPresses *buttSwitchEXT[] = {nullptr};

void startSwitch()
{
        windowSwitch.pin0 = inputUpPin;
        windowSwitch.pin1 = inputDownPin;
        windowSwitch.buttonType = 2;
        // windowSwitch.buttonType = 200;
        windowSwitch.start();

        if (useExtInput)
        {
                static buttonPresses windowSwitchExt;
                buttSwitchEXT[0] = &windowSwitchExt;

                buttSwitchEXT[0]->pin0 = inputUpExtPin;
                buttSwitchEXT[0]->pin1 = inputDownExtPin;
                buttSwitchEXT[0]->buttonType = 2;
                buttSwitchEXT[0]->start();
        }
}
void readSwitch()
{
        uint8_t readSW = windowSwitch.getValue();
        if (readSW != 0)
        {
                switchIt("Switch", readSW);
        }
        if (useExtInput)
        {
                uint8_t readExt = buttSwitchEXT[0]->getValue();
                if (readExt != 0)
                {
                        switchIt("ext-Switch", readExt);
                }
        }
}
void startGPIOs()
{
        pinMode(outputUpPin, OUTPUT);
        pinMode(outputDownPin, OUTPUT);
        allOff();
}
void allOff()
{
        digitalWrite(outputUpPin, !RelayOn);
        digitalWrite(outputDownPin, !RelayOn);
        delay(50);
}
void switchIt(char *type, uint8_t dir)
{
        char mqttmsg[50];
        const char *st[] = {"nan", "up", "down", "off"};

        if (dir == 3)
        {
                allOff();
        }
        else if (dir == 2)
        {
                allOff();
                digitalWrite(outputDownPin, RelayOn);
        }
        else if (dir == 1)
        {
                allOff();
                digitalWrite(outputUpPin, RelayOn);
        }
        
        sprintf(mqttmsg,"%s",st[dir]);
        iot.pub_state(mqttmsg);
        sprintf(mqttmsg, "%s: Switched [%s]", type, st[dir]);
        iot.pub_msg(mqttmsg);

        if (useAutoRelayOFF)
        {
                if (dir <= 2)
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
void checkTimeout_AutoRelay_Off(int timeout_off)
{
        if (useAutoRelayOFF)
        {
                if (autoOff_clock != 0 && millis() - autoOff_clock > timeout_off * 1000)
                {
                        switchIt("timeout", 3);
                        autoOff_clock = 0;
                }
        }
}
void verifyNotHazardState()
{
        if (digitalRead(outputUpPin) == RelayOn && digitalRead(outputDownPin) == RelayOn)
        {
                switchIt("Button", 3);
                iot.sendReset("HazradState");
        }
}

void setup()
{
        startRead_parameters();
        startGPIOs();
        startSwitch();
        startIOTservices();
        endRead_parameters();
}
void loop()
{
        iot.looper();
        verifyNotHazardState(); // both up and down are ---> OFF
        readSwitch();
        checkTimeout_AutoRelay_Off(AutoRelayOff_timeout);
        // delay(100); /* No need - delays in readSwitch 100ms */
}
