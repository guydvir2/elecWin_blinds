#include <ArduinoJson.h>
#include <TimeLib.h>

// ~~~~ Services update via ESP on BOOT ~~~~~
bool DUAL_SW = false;
bool ERR_PROTECT = true;
bool USE_TO = true;
uint8_t TO_DURATION = 60;
time_t bootTime;
int del_loop = 2;  // millis
int del_off = 100; // millis
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define WAIT_FOR_PARAM_DURATION 15
#define JSON_SERIAL_SIZE 300

#define VER "Arduino_v1.32"
#define MCU_TYPE "ProMini"
#define DEV_NAME "MCU"

// #define SW_DOWN_PIN 2   /* Switch1 INPUT to Arduino */
// #define SW_UP_PIN 3     /* Switch1 INPUT to Arduino */
// #define SW2_DOWN_PIN 4  /* Switch2 INPUT to Arduino */
// #define SW2_UP_PIN 5    /* Switch2 INPUT to Arduino */
// #define REL_DOWN_PIN 10 /* OUTUPT to relay device */
// #define REL_UP_PIN 11   /* OUTUPT to relay device */

#define SW_DOWN_PIN 4   /* Switch1 INPUT to Arduino */
#define SW_UP_PIN 5     /* Switch1 INPUT to Arduino */
#define SW2_DOWN_PIN 4  /* Switch2 INPUT to Arduino */
#define SW2_UP_PIN 5    /* Switch2 INPUT to Arduino */
#define REL_DOWN_PIN 3 /* OUTUPT to relay device */
#define REL_UP_PIN 2   /* OUTUPT to relay device */

#define RELAY_ON LOW
#define SW_PRESSED LOW

const uint8_t debounce_delay = 50;  //ms
const uint8_t MIN2RESET_BAD_P = 30; // Minutes to reset due to not getting Remote Parameters

unsigned long autoOff_clock = 0;
bool getP_OK = false; // Flag, external parameters got OK ?

bool swUp_lastState = !SW_PRESSED;
bool swDown_lastState = !SW_PRESSED;
bool swUp2_lastState = !SW_PRESSED;
bool swDown2_lastState = !SW_PRESSED;

enum sys_states : const uint8_t
{
  WIN_STOP,
  WIN_UP,
  WIN_DOWN,
  WIN_ERR,
};

void (*resetFunc)(void) = 0;

// ~~~~~~~~~  Serial Communication ~~~~~~~~
void sendMSG(char *msg, char *addinfo = NULL)
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
  serializeJson(doc, Serial);
}
void Serial_CB(JsonDocument &_doc)
{
  const char *FROM = _doc["from"];
  const char *ACT = _doc["act"];
  const char *INFO = _doc["info"];

  if (strcmp(ACT, "up") == 0)
  {
    if (makeSwitch(WIN_UP))
    {
      sendMSG(ACT, INFO);
    }
    // else
    // {
    //   sendMSG("error", "window state");
    // }
  }
  else if (strcmp(ACT, "down") == 0)
  {
    if (makeSwitch(WIN_DOWN))
    {
      sendMSG(ACT, INFO);
    }
    // else
    // {
    //   sendMSG("error", "window state");
    // }
  }
  else if (strcmp(ACT, "off") == 0)
  {
    if (makeSwitch(WIN_STOP))
    {
      sendMSG(ACT, INFO);
    }
    // else
    // {
    //   sendMSG("error", "window state");
    // }
  }
  else if (strcmp(ACT, "reset_MCU") == 0)
  {
    resetFunc();
  }
  else if (strcmp(ACT, "status") == 0)
  {
    uint8_t a = getWin_state();
    switch (a)
    {
    case WIN_DOWN:
      sendMSG("status", "down");
      break;
    case WIN_UP:
      sendMSG("status", "up");
      break;
    case WIN_STOP:
      sendMSG("status", "off");
      break;

    default:
      sendMSG("status", "error-state");
      break;
    }
  }
  else if (strcmp(ACT, "query") == 0)
  {
    char t[200];
    char clk2[25];
    sprintf(clk2, "%02d-%02d-%02d %02d:%02d:%02d", year(bootTime), month(bootTime), day(bootTime), hour(bootTime), minute(bootTime), second(bootTime));
    sprintf(t, "ver[%s], MCU[%s], DualSW[%s], ErrProtect[%s], bootTime[%s],Auto-Off[%s], Auto-Off_TO[%d],%d,%d",
            VER, MCU_TYPE, DUAL_SW ? "YES" : "NO", ERR_PROTECT ? "YES" : "NO", clk2, USE_TO ? "YES" : "NO", TO_DURATION, del_off, del_loop);
    sendMSG("query", t);
  }
  else if (strcmp(ACT, "boot_p") == 0)
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
    sendMSG("error", output);
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
      sendMSG("error", "Serial-Recv");
    }
  }
}
void getRemote_param(uint8_t _waitDuration = 15)
{
  sendMSG("boot_p");                                          /* calling for remote parameters */
  while (millis() < _waitDuration * 1000 && getP_OK == false) /* Wait to get parameters */
  {
    if (Serial.available() > 0)
    {
      StaticJsonDocument<JSON_SERIAL_SIZE> doc;
      DeserializationError error = deserializeJson(doc, Serial);
      if (!error)
      {
        const char *ACT = doc["act"];
        if (strcmp(ACT, "boot_p") == 0)
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
        sendMSG("boot_p");
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
  sendMSG("Boot");
  if (year(bootTime) == 1970)
  {
    sendMSG("error", "NTP");
  }
  if (getP_OK == false)
  {
    sendMSG("error", "Parameters");
  }
}

// ~~~~~~ Handling Inputs & Outputs ~~~~~~~
void start_gpio()
{
  pinMode(REL_UP_PIN, OUTPUT);
  pinMode(REL_DOWN_PIN, OUTPUT);
  pinMode(SW_UP_PIN, INPUT_PULLUP);
  pinMode(SW_DOWN_PIN, INPUT_PULLUP);
  swUp_lastState = digitalRead(SW_UP_PIN);
  swDown_lastState = digitalRead(SW_DOWN_PIN);

  if (DUAL_SW)
  {
    pinMode(SW2_UP_PIN, INPUT_PULLUP);
    pinMode(SW2_DOWN_PIN, INPUT_PULLUP);
    swUp2_lastState = digitalRead(SW2_UP_PIN);
    swDown2_lastState = digitalRead(SW2_DOWN_PIN);
  }
  allOff();
}
void allOff()
{
  digitalWrite(REL_UP_PIN, !RELAY_ON);
  digitalWrite(REL_DOWN_PIN, !RELAY_ON);
  delay(del_off);
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
    switch (state)
    {
    case WIN_STOP:
      allOff();
      break;
    case WIN_UP:
      allOff();
      digitalWrite(REL_UP_PIN, RELAY_ON);
      break;
    case WIN_DOWN:
      allOff();
      digitalWrite(REL_DOWN_PIN, RELAY_ON);
      break;
    default:
      allOff();
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
void errorProtection()
{
  if (ERR_PROTECT)
  {
    if (digitalRead(REL_UP_PIN) == RELAY_ON && digitalRead(REL_DOWN_PIN) == RELAY_ON)
    {
      makeSwitch(WIN_STOP);
      sendMSG("error", "Relays");
    }
    if (digitalRead(SW_UP_PIN) == SW_PRESSED && digitalRead(SW_DOWN_PIN) == SW_PRESSED)
    {
      sendMSG("error", "Buttons");
      delay(100);
      resetFunc();
    }
    if (DUAL_SW)
    {
      if (digitalRead(SW2_UP_PIN) == SW_PRESSED && digitalRead(SW2_DOWN_PIN) == SW_PRESSED)
      {
        sendMSG("error", "ExButtons");
        delay(100);
        resetFunc();
      }
    }
  }
}
void act_inputChange(int inPin, bool &state)
{
  if (state == SW_PRESSED)
  {
    if (inPin == SW_UP_PIN || inPin == SW2_UP_PIN)
    {
      if (makeSwitch(WIN_UP))
      {
        if (inPin == SW_UP_PIN)
        {
          sendMSG("up", "Button");
        }
        else
        {
          sendMSG("up", "ExButton");
        }
      }
      else
      {
        sendMSG("error", "window state");
      }
    }
    else if (inPin == SW_DOWN_PIN || inPin == SW2_DOWN_PIN)
    {
      if (makeSwitch(WIN_DOWN))
      {
        if (inPin == SW_DOWN_PIN)
        {
          sendMSG("down", "Button");
        }
        else
        {
          sendMSG("down", "ExButton");
        }
      }
      else
      {
        sendMSG("error", "window state");
      }
    }
  }
  else
  {
    if (makeSwitch(WIN_STOP))
    {
      if (inPin == SW_DOWN_PIN || inPin == SW_UP_PIN)
      {
        sendMSG("off", "Button");
      }
      else
      {
        sendMSG("off", "ExButton");
      }
    }
    else
    {
      sendMSG("error", "window state");
    }
  }
}
void readInput(int inPin, bool &lastState)
{
  bool state = digitalRead(inPin);
  if (state != lastState)
  {
    delay(debounce_delay);
    if (digitalRead(inPin) == state)
    {
      act_inputChange(inPin, state);
      lastState = state;
    }
  }
}
void readInputs_looper()
{
  readInput(SW_UP_PIN, swUp_lastState);     /* Read wall UP Switch */
  readInput(SW_DOWN_PIN, swDown_lastState); /* Read wall DOWN Switch */
  if (DUAL_SW)
  {
    readInput(SW2_UP_PIN, swUp2_lastState);     /* Read wall UP Switch */
    readInput(SW2_DOWN_PIN, swDown2_lastState); /* Read wall DOWN Switch */
  }
}
void autoOff_looper()
{
  if (USE_TO && autoOff_clock != 0)
  {
    if (millis() > autoOff_clock + TO_DURATION * 1000UL)
    {
      makeSwitch(WIN_STOP);
      sendMSG("off", "Auto-Off");
    }
  }
}
void reset_fail_load_parameters()
{
  if (getP_OK == false && millis() > MIN2RESET_BAD_P * 1000UL * 60)
  {
    sendMSG("error", "Reset");
    delay(1000);
    resetFunc();
  }
}

void setup()
{
  Serial.begin(9600);
  start_gpio();
  getRemote_param(WAIT_FOR_PARAM_DURATION);
  postBoot_err_notification();
}
void loop()
{
  readInputs_looper();
  errorProtection(); /* Avoid Simulatnious UP&DOWN */
  readSerial();
  autoOff_looper();
  reset_fail_load_parameters();
  delay(del_loop);
}
