#include <ArduinoJson.h>
#include <TimeLib.h>
#include <buttonPresses.h>
#include <Arduino.h>

#define VER "Arduino_v1.45"
#define MCU_TYPE "ProMini"
#define DEV_NAME "MCU"
#define RELAY_ON LOW
#define SW_PRESSED LOW
#define SW_DOWN_PIN 2   //4  /* Switch1 INPUT to Arduino */
#define SW_UP_PIN 3     //5  /* Switch1 INPUT to Arduino */
#define SW2_DOWN_PIN 4  /* Switch2 INPUT to Arduino */
#define SW2_UP_PIN 5    /* Switch2 INPUT to Arduino */
#define REL_DOWN_PIN 10 //3  /* OUTUPT to relay device */
#define REL_UP_PIN 11   //2  /* OUTUPT to relay device */
#define TIMEOUT_PARAM 15
#define JSON_SERIAL_SIZE 450
#define MILLIS_TO_MINUTES 60000UL
#define BOOT_DELAY 10000

buttonPresses buttSwitch;
buttonPresses *buttSwitchEXT[] = {nullptr, nullptr};

// ~~~~ Services update via ESP on BOOT ~~~~~
bool DUAL_SW = false;
bool ERR_PROTECT = true;
bool USE_TO = true;
bool useAlive = false;
uint8_t TO_DURATION = 60;
uint8_t del_loop = 2;  // millis
uint8_t del_off = 100; // millis
uint8_t btype_2 = 2;   // Button Type
uint8_t Alive_int = 10;
time_t bootTime;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool getP_OK = false;               /* Flag, external parameters got OK ? */
const uint8_t MIN2RESET_BAD_P = 30; /* Minutes to reset due to not getting Remote Parameters */
unsigned long autoOff_clock = 0;
const char *winStates[] = {"Error", "up", "down", "off"};
const char *msgKW[] = {"from", "type", "info", "info2"};
const char *msgTypes[] = {"act", "info", "error"};
const char *msgAct[] = {winStates[1], winStates[2], winStates[3], "reset_MCU", "Auto-Off"};
const char *msgInfo[] = {"status", "query", "boot_p", "Boot", "error", "ping", "button", "MQTT", "confirm"};
const char *msgErrs[] = {"Comm", "Parameters", "Boot", "unKnown-error"};

enum sys_states : const uint8_t
{
  WIN_ERR,
  WIN_UP,
  WIN_DOWN,
  WIN_STOP,
};
void (*resetFunc)(void) = 0;

// ~~~~~~~~~  Serial Communication ~~~~~~~~
void _constMSG(JsonDocument &doc, const char *KW1, const char *KW2, const char *KW3)
{
  doc[msgKW[0]] = DEV_NAME;
  doc[msgKW[1]] = KW1;
  doc[msgKW[2]] = KW2;
  doc[msgKW[3]] = KW3;
}
void _sendMSG(JsonDocument &_doc)
{
  serializeJson(_doc, Serial);
}
void sendMSG(char *msgtype, char *ext1, char *ext2 = "0")
{
  StaticJsonDocument<JSON_SERIAL_SIZE> doc;

  _constMSG(doc, msgtype, ext1, ext2);
  _sendMSG(doc);
}

void _update_bootP(JsonDocument &_doc)
{
  ERR_PROTECT = _doc["err_p"];
  DUAL_SW = _doc["dub_sw"];
  USE_TO = _doc["t_out"];
  TO_DURATION = _doc["t_out_d"];
  bootTime = _doc["boot_t"];
  del_loop = _doc["del_loop"];
  del_off = _doc["del_off"];
  btype_2 = _doc["btype_2"]; /* Button type for external input only. This part is not solved yet */
  Alive_int = _doc["Alive_int"];
  useAlive = _doc["useAlive"];
}
void _replyStatus()
{
  uint8_t a = getWin_state();
  switch (a)
  {
  case WIN_UP:
    sendMSG(msgTypes[1], msgInfo[0], msgAct[0]);
    break;
  case WIN_DOWN:
    sendMSG(msgTypes[1], msgInfo[0], msgAct[1]);
    break;
  case WIN_STOP:
    sendMSG(msgTypes[1], msgInfo[0], msgAct[2]);
    break;

  default:
    sendMSG(msgTypes[1], msgInfo[0], msgInfo[4]);
    break;
  }
}
void _replyQuery()
{
  char t[250];
  char clk2[25];
  sprintf(clk2, "%02d-%02d-%02d %02d:%02d:%02d", year(bootTime), month(bootTime), day(bootTime), hour(bootTime), minute(bootTime), second(bootTime));
  sprintf(t, "ver[%s], MCU[%s], DualSW[%s], Parameters[%s], ErrProtect[%s], bootTime[%s],Auto-Off[%s], Auto-Off_TO[%d],%d,%d",
          VER, MCU_TYPE, DUAL_SW ? "YES" : "NO", getP_OK ? "YES" : "NO", ERR_PROTECT ? "YES" : "NO", clk2, USE_TO ? "YES" : "NO",
          TO_DURATION, del_off, del_loop);
  sendMSG(msgTypes[1], msgKW[2], t);
}
void _Actions_cb(const char *KW2)
{
  if (strcmp(KW2, msgAct[0]) == 0)       /* Window UP */
  {
    makeSwitch(WIN_UP);
  }
  else if (strcmp(KW2, msgAct[1]) == 0)  /* Window DOWN */
  {
    makeSwitch(WIN_DOWN);
  }
  else if (strcmp(KW2, msgAct[2]) == 0)  /* Window OFF */
  {
    makeSwitch(WIN_STOP);
  }
  else if (strcmp(KW2, msgAct[3]) == 0)  /* init Reset */
  {
    resetFunc();
  }
  sendMSG(msgTypes[1], KW2, msgInfo[8]); /* Notify Action was done */
}
void _Infos_cb(const char *KW2, JsonDocument &_doc)
{
  if (strcmp(KW2, msgInfo[0]) == 0)      /* Status */
  {
    _replyStatus();
  }
  else if (strcmp(KW2, msgInfo[1]) == 0) /* Query*/
  {
    _replyQuery();
  }
  else if (strcmp(KW2, msgInfo[2]) == 0) /* boot Parameters */
  {
    _update_bootP(_doc);
  }
}
void Serial_CB(JsonDocument &_doc)
{
  const char *FROM = _doc[msgKW[0]];
  const char *TYPE = _doc[msgKW[1]];
  const char *KW2 = _doc[msgKW[2]];
  const char *KW3 = _doc[msgKW[3]];

  if (strcmp(TYPE, msgTypes[0]) == 0) /* Got Actions */
  {
    _Actions_cb(KW2);
  }
  else if (strcmp(TYPE, msgTypes[1]) == 0) /* info */
  {
    _Infos_cb(KW2, _doc);
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
      Serial_CB(doc); /* send message to Callback */
    }
    else
    {
      sendMSG(msgTypes[2], msgErrs[0]); /* Communication Error */
    }
  }
}

void request_remoteParameters(uint8_t _waitDuration = 15)
{
  sendMSG(msgTypes[1], msgInfo[2]);                           /* calling for remote parameters */
  while (millis() < _waitDuration * 1000 && getP_OK == false) /* Wait to get parameters */
  {
    if (Serial.available() > 0)
    {
      StaticJsonDocument<JSON_SERIAL_SIZE> doc;
      DeserializationError error = deserializeJson(doc, Serial);
      if (!error)
      {
        const char *TYPE = doc[msgKW[1]];
        const char *KW2 = doc[msgKW[2]];

        if (strcmp(TYPE, msgTypes[1]) == 0) /* verify boot_parameters */
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
        sendMSG(msgTypes[1], msgInfo[2]); /* Send request again */
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
  sendMSG(msgTypes[1], msgInfo[3]);
  if (year(bootTime) == 1970)
  {
    sendMSG(msgTypes[2], msgInfo[3], "NTP");
  }
  if (getP_OK == false)
  {
    sendMSG(msgTypes[2], msgInfo[3], "Parameters");
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

  if (DUAL_SW)
  {
    static buttonPresses buttSwitchExt;
    buttSwitchEXT[0] = &buttSwitchExt;
    buttSwitchEXT[0]->pin0 = SW2_UP_PIN;
    buttSwitchEXT[0]->pin1 = SW2_DOWN_PIN;
    buttSwitchEXT[0]->buttonType = btype_2;
    buttSwitchEXT[0]->start();
  }
}
void read_buttSwitch()
{
  uint8_t switchRead = buttSwitch.getValue();
  if (switchRead != 0)
  {
    /*  0: no request; 1: request up; 2: request down; 3: request off */
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
    sendMSG(msgTypes[0], msgAct[value - 1], msgInfo[6]);
  }
}

void autoOff_looper()
{
  if (USE_TO && autoOff_clock != 0)
  {
    if (millis() > autoOff_clock + TO_DURATION * 1000UL)
    {
      makeSwitch(WIN_STOP);
      sendMSG(msgTypes[0], winStates[3], msgAct[4]);
    }
  }
}
void reset_fail_load_parameters()
{
  if (getP_OK == false && millis() > MIN2RESET_BAD_P * MILLIS_TO_MINUTES)
  {
    sendMSG(msgTypes[2], msgErrs[1]);
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
      sendMSG(msgTypes[2], "Relays");
    }
    if (digitalRead(SW_UP_PIN) == SW_PRESSED && digitalRead(SW_DOWN_PIN) == SW_PRESSED)
    {
      sendMSG(msgTypes[2], "Buttons");
      delay(100);
      resetFunc();
    }
    if (DUAL_SW)
    {
      if (digitalRead(SW2_UP_PIN) == SW_PRESSED && digitalRead(SW2_DOWN_PIN) == SW_PRESSED)
      {
        sendMSG(msgTypes[2], "ExButtons");
        delay(100);
        resetFunc();
      }
    }
  }
}

void setup()
{
  start_gpio();
  start_buttSW();
  Serial.begin(9600);
  Serial.flush();
  delay(BOOT_DELAY);
  request_remoteParameters(TIMEOUT_PARAM);
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