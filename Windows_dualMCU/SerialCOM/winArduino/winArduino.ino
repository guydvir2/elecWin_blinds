#include <ArduinoJson.h>
#include <TimeLib.h>
#include <buttonPresses.h>
#include <Arduino.h>

#define VER "Arduino_v1.4"
#define MCU_TYPE "ProMini"
#define DEV_NAME "MCU"
#define RELAY_ON LOW
#define SW_PRESSED LOW
#define SW_DOWN_PIN 2   /* Switch1 INPUT to Arduino */
#define SW_UP_PIN 3     /* Switch1 INPUT to Arduino */
#define SW2_DOWN_PIN 4  /* Switch2 INPUT to Arduino */
#define SW2_UP_PIN 5    /* Switch2 INPUT to Arduino */
#define REL_DOWN_PIN 10 /* OUTUPT to relay device */
#define REL_UP_PIN 11   /* OUTUPT to relay device */
#define TIMEOUT_PARAM 15
#define JSON_SERIAL_SIZE 300

buttonPresses buttSwitch;
buttonPresses *buttSwitchEXT[] = {nullptr, nullptr};

// ~~~~ Services update via ESP on BOOT ~~~~~
bool DUAL_SW = false;
bool ERR_PROTECT = true;
bool USE_TO = true;
uint8_t TO_DURATION = 60;
uint8_t del_loop = 2;  // millis
uint8_t del_off = 100; // millis
uint8_t btype_2 = 2;   // Button Type
time_t bootTime;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// #define SW_DOWN_PIN 4  /* Switch1 INPUT to Arduino */
// #define SW_UP_PIN 5    /* Switch1 INPUT to Arduino */
// #define SW2_DOWN_PIN 4 /* Switch2 INPUT to Arduino */
// #define SW2_UP_PIN 5   /* Switch2 INPUT to Arduino */
// #define REL_DOWN_PIN 3 /* OUTUPT to relay device */
// #define REL_UP_PIN 2   /* OUTUPT to relay device */

bool getP_OK = false;               /* Flag, external parameters got OK ? */
const uint8_t MIN2RESET_BAD_P = 30; /* Minutes to reset due to not getting Remote Parameters */
unsigned long autoOff_clock = 0;
const char *winStates[] = {"Error", "up", "down", "off"};
const char *serialKW[] = {"from", "act", "info", "error"};
const char *serialCMD[] = {"status", "reset_MCU", "query", "boot_p", "Boot", "error"};
enum sys_states : const uint8_t
{
  WIN_ERR,
  WIN_UP,
  WIN_DOWN,
  WIN_STOP,
};
void (*resetFunc)(void) = 0;

// ~~~~~~~~~  Serial Communication ~~~~~~~~
// void msgCenter()
void sendMSG(char *msg, char *addinfo = NULL)
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
  serializeJson(doc, Serial);
}
void Serial_CB(JsonDocument &_doc)
{
  const char *FROM = _doc[serialKW[0]];
  const char *ACT = _doc[serialKW[1]];
  const char *INFO = _doc[serialKW[2]];

  if (strcmp(ACT, winStates[1]) == 0)
  {
    if (makeSwitch(WIN_UP))
    {
      sendMSG(ACT, INFO);
    }
  }
  else if (strcmp(ACT, winStates[2]) == 0)
  {
    if (makeSwitch(WIN_DOWN))
    {
      sendMSG(ACT, INFO);
    }
  }
  else if (strcmp(ACT, winStates[3]) == 0)
  {
    if (makeSwitch(WIN_STOP))
    {
      sendMSG(ACT, INFO);
    }
  }
  else if (strcmp(ACT, serialCMD[0]) == 0)
  {
    uint8_t a = getWin_state();
    switch (a)
    {
    case WIN_DOWN:
      sendMSG(serialCMD[0], winStates[2]);
      break;
    case WIN_UP:
      sendMSG(serialCMD[0], winStates[1]);
      break;
    case WIN_STOP:
      sendMSG(serialCMD[0], winStates[3]);
      break;

    default:
      sendMSG(serialCMD[0], winStates[0]);
      break;
    }
  }
  else if (strcmp(ACT, serialCMD[1]) == 0)
  {
    resetFunc();
  }
  else if (strcmp(ACT, serialCMD[2]) == 0)
  {
    char t[200];
    char clk2[25];
    sprintf(clk2, "%02d-%02d-%02d %02d:%02d:%02d", year(bootTime), month(bootTime), day(bootTime), hour(bootTime), minute(bootTime), second(bootTime));
    sprintf(t, "ver[%s], MCU[%s], DualSW[%s], ErrProtect[%s], bootTime[%s],Auto-Off[%s], Auto-Off_TO[%d],%d,%d",
            VER, MCU_TYPE, DUAL_SW ? "YES" : "NO", ERR_PROTECT ? "YES" : "NO", clk2, USE_TO ? "YES" : "NO", TO_DURATION, del_off, del_loop);
    sendMSG(serialCMD[2], t);
  }
  else if (strcmp(ACT, serialCMD[3]) == 0)
  {
    // ~~~ Update sketch parameters ~~~
    ERR_PROTECT = _doc["err_p"];
    DUAL_SW = _doc["dub_sw"];
    USE_TO = _doc["t_out"];
    TO_DURATION = _doc["t_out_d"];
    bootTime = _doc["boot_t"];
    del_loop = _doc["del_loop"];
    del_off = _doc["del_off"];
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  }
  else
  {
    char output[120];
    serializeJson(_doc, output);
    sendMSG(serialKW[3], output);
  }
}
void readSerial()
{
  if (Serial.available() > 0)
  {
    StaticJsonDocument<JSON_SERIAL_SIZE> doc;
    DeserializationError error = deserializeJson(doc, Serial);
    if (!error)
    {
      Serial_CB(doc);
    }
    else
    {
      sendMSG(serialCMD[5], "Recv");
    }
  }
}
void getRemote_param(uint8_t _waitDuration = 15)
{
  sendMSG(serialCMD[3]);                                      /* calling for remote parameters */
  while (millis() < _waitDuration * 1000 && getP_OK == false) /* Wait to get parameters */
  {
    if (Serial.available() > 0)
    {
      StaticJsonDocument<JSON_SERIAL_SIZE> doc;
      DeserializationError error = deserializeJson(doc, Serial);
      if (!error)
      {
        const char *ACT = doc[serialKW[1]];
        if (strcmp(ACT, serialCMD[3]) == 0) /* verify boot_parameters */
        {
          Serial_CB(doc);
          getP_OK = true;
        }
      }
    }
    else
    {
      static uint8_t loopCounter = 0; /* This part is needed when ESP has not completed boot process */
      if (loopCounter > 5)
      {
        delay(1000);
        sendMSG(serialCMD[3]);
      }
      else
      {
        loopCounter++;
      }
    }
    delay(50);
  }
}
void postBoot_err_notification()
{
  sendMSG(serialCMD[4]);
  if (year(bootTime) == 1970)
  {
    sendMSG(serialKW[3], "NTP");
  }
  if (getP_OK == false)
  {
    sendMSG(serialKW[3], "Parameters");
  }
}

// ~~~~~~ Handling Inputs & Outputs ~~~~~~~
void allOff()
{
  digitalWrite(REL_UP_PIN, !RELAY_ON);
  digitalWrite(REL_DOWN_PIN, !RELAY_ON);
  delay(del_off);
}
void start_gpio()
{
  pinMode(REL_UP_PIN, OUTPUT);
  pinMode(REL_DOWN_PIN, OUTPUT);
  allOff();
}
void start_buttSW()
{
  buttSwitch.pin0 = SW_UP_PIN;
  buttSwitch.pin1 = SW_DOWN_PIN;
  buttSwitch.buttonType = 2;
  buttSwitch.start();

  static buttonPresses buttSwitchExt;

  if (DUAL_SW)
  {
    buttSwitchEXT[0] = &buttSwitchExt;
    buttSwitchEXT[0]->pin0 = SW2_UP_PIN;
    buttSwitchEXT[0]->pin1 = SW2_DOWN_PIN;
    buttSwitchEXT[0]->buttonType = 2;
    buttSwitchEXT[0]->start();
  }
}
void read_buttSwitch()
{
  uint8_t switchRead = buttSwitch.getValue();
  if (switchRead != 0)
  {
    /*  0: no change; 1: up; 2: down; 3: off */
    switch_cb(switchRead);
  }
}

uint8_t getWin_state()
{
  bool relup = digitalRead(REL_UP_PIN);
  bool reldown = digitalRead(REL_DOWN_PIN);

  if (relup == !RELAY_ON && reldown == !RELAY_ON)
  {
    return WIN_STOP;
  }
  else if (relup == RELAY_ON && reldown == !RELAY_ON)
  {
    return WIN_UP;
  }
  else if (relup == !RELAY_ON && reldown == RELAY_ON)
  {
    return WIN_DOWN;
  }
  else
  {
    return WIN_ERR;
  }
}
bool makeSwitch(uint8_t state)
{
  if (getWin_state() != state) /* Not already in that state */
  {
    allOff();
    switch (state)
    {
    case WIN_STOP:
      break;
    case WIN_UP:
      digitalWrite(REL_UP_PIN, RELAY_ON);
      break;
    case WIN_DOWN:
      digitalWrite(REL_DOWN_PIN, RELAY_ON);
      break;
    default:
      break;
    }

    if (USE_TO)
    {
      if (state != WIN_STOP)
      {
        autoOff_clock = millis();
      }
      else
      {
        autoOff_clock = 0;
      }
    }
    return 1;
  }
  else
  {
    return 0;
  }
}
void switch_cb(uint8_t value)
{
  if (makeSwitch(value))
  {
    sendMSG(winStates[value], "Button");
  }
}

void autoOff_looper()
{
  if (USE_TO && autoOff_clock != 0)
  {
    if (millis() > autoOff_clock + TO_DURATION * 1000UL)
    {
      makeSwitch(WIN_STOP);
      sendMSG(winStates[3], "Auto-Off");
    }
  }
}
void reset_fail_load_parameters()
{
  if (getP_OK == false && millis() > MIN2RESET_BAD_P * 1000UL * 60)
  {
    sendMSG(serialCMD[5], "Reset");
    delay(1000);
    resetFunc();
  }
}
void errorProtection()
{
  if (ERR_PROTECT)
  {
    if (digitalRead(REL_UP_PIN) == RELAY_ON && digitalRead(REL_DOWN_PIN) == RELAY_ON)
    {
      makeSwitch(WIN_STOP);
      sendMSG(serialKW[3], "Relays");
    }
    if (digitalRead(SW_UP_PIN) == SW_PRESSED && digitalRead(SW_DOWN_PIN) == SW_PRESSED)
    {
      sendMSG(serialKW[3], "Buttons");
      delay(100);
      resetFunc();
    }
    if (DUAL_SW)
    {
      if (digitalRead(SW2_UP_PIN) == SW_PRESSED && digitalRead(SW2_DOWN_PIN) == SW_PRESSED)
      {
        sendMSG(serialKW[3], "ExButtons");
        delay(100);
        resetFunc();
      }
    }
  }
}

void setup()
{
  Serial.begin(9600);
  start_gpio();
  start_buttSW(); // <---- NEW
  getRemote_param(TIMEOUT_PARAM);
  postBoot_err_notification();
}
void loop()
{
  read_buttSwitch();
  errorProtection(); /* Avoid Simulatnious UP&DOWN */
  readSerial();
  autoOff_looper();
  reset_fail_load_parameters();
  delay(del_loop);
}
