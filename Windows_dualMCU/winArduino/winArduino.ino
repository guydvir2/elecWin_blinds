#include <ArduinoJson.h>
#include <buttonPresses.h>
#include <Arduino.h>
#include <time.h>

// #define DEBUG_MODE true
// #if DEBUG_MODE
// #include <SoftwareSerial.h>
// SoftwareSerial mySerial(9, 8); // RX, TX
// #endif

#define VER "Arduino_v1.6_beta"
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
#define JSON_SERIAL_SIZE 200

buttonPresses buttSwitch;
buttonPresses *buttSwitchEXT[] = {nullptr, nullptr};

// ~~~~ Services update via ESP on BOOT ~~~~~
bool DualSW = false;     /* 2 Switches Window*/
bool Err_Protect = true; /* Monitor UP&DOWN pressed together*/
bool AutoOff = true;     /* Timeout to switch off Relays */
bool Lockdown = false;   /* lock operations of relays, both MQTT and Switch */

uint8_t AutoOff_duration = 60;
uint8_t btype_2 = 2; // Button Type
time_t bootTime;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool lockdown_state = false; /* Flag, lockdown command */
bool getP_OK = false;        /* Flag, external parameters got OK ? */
unsigned long autoOff_clock = 0;
const uint8_t MIN2RESET_BAD_P = 30; /* Minutes to reset due to not getting Remote Parameters */
const char *winStates[] = {"Error", "up", "down", "off"};
const char *msgKW[] = {"from", "type", "i", "i_ext"};
const char *msgTypes[] = {"act", "info", "error"};
const char *msgAct[] = {winStates[0], winStates[1], winStates[2], winStates[3], "reset_MCU", "Auto-Off", "lockdown_on", "lockdown_off"};
const char *msgInfo[] = {"status", "query", "boot_p", "Boot", "error", "button", "MQTT", "ping"};
const char *msgErrs[] = {"Comm", "Parameters", "Boot", "unKnown-error"};

enum sys_states : const uint8_t
{
  WIN_ERR,
  WIN_UP,
  WIN_DOWN,
  WIN_STOP,
};
void (*resetFunc)(void) = 0;

// ~~~~~~~~~ Serial Communication ~~~~~~~~
void _constructMSG(JsonDocument &doc, const char *KW1, const char *KW2, const char *KW3)
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
void sendMSG(const char *msgtype, const char *ext1, const char *ext2 = "0")
{
  StaticJsonDocument<JSON_SERIAL_SIZE> doc;

  _constructMSG(doc, msgtype, ext1, ext2);
  _sendMSG(doc);
  // #if DEBUG_MODE
  //   Serial.print("\nSent: ");
  //   serializeJson(doc, Serial);
  // #endif
}
void switch_cb(uint8_t value, char *src)
{
  if (!checkLockdown())
  {
    if (_makeSwitch(value))
    {
      sendMSG(msgTypes[0], msgAct[value], src);
    }
  }
  else /* in LOCKDOWN mode */ 
  {
    if (_makeSwitch(WIN_STOP)) /* Allow AutoOff*/
    {
      sendMSG(msgTypes[0], msgAct[WIN_STOP], src);
    }
  }
}

void _replyStatus()
{
  uint8_t a = getWin_state();
  if (a == 0)
  {
    sendMSG(msgTypes[1], msgInfo[0], msgInfo[4]);
  }
  else
  {
    sendMSG(msgTypes[1], msgInfo[0], msgAct[a]);
  }
}
void _replyQuery()
{
  char t[250];
  char clk2[25];
  uint8_t day_light = 3;
  struct tm *tm = localtime(&bootTime);
  if (tm->tm_mon >= 10 && tm->tm_mon < 4)
  {
    day_light = 2;
  }

  sprintf(clk2, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour + day_light, tm->tm_min, tm->tm_sec);

  sprintf(t, "ver[%s], MCU[%s], DualSW[%s], BootP[%s], eProtect[%s], boot[%s],Auto_off[%s %dsec], Lockdown[%d]",
          VER, MCU_TYPE, DualSW ? "YES" : "NO", getP_OK ? "OK" : "FAIL", Err_Protect ? "YES" : "NO", clk2, AutoOff ? "YES" : "NO", AutoOff_duration,Lockdown);
  sendMSG(msgTypes[1], msgInfo[1], t);
}
void _Actions_cb(const char *KW2)
{
  if (strcmp(KW2, msgAct[WIN_UP]) == 0) /* Window UP */
  {
    switch_cb(WIN_UP, msgInfo[6]);
  }
  else if (strcmp(KW2, msgAct[WIN_DOWN]) == 0) /* Window DOWN */
  {
    switch_cb(WIN_DOWN, msgInfo[6]);
  }
  else if (strcmp(KW2, msgAct[WIN_STOP]) == 0) /* Window OFF */
  {
    switch_cb(WIN_STOP, msgInfo[6]);
  }
  else if (strcmp(KW2, msgAct[4]) == 0) /* init Reset */
  {
    resetFunc();
  }
  else if (strcmp(KW2, msgAct[6]) == 0) /* lockdown ON */
  {
    update_lockdown_state(true);
  }
  else if (strcmp(KW2, msgAct[7]) == 0) /* lockdown OFF */
  {
    update_lockdown_state(false);
  }
}
void _Infos_cb(JsonDocument &_doc)
{
  if (strcmp(_doc[msgKW[2]], msgInfo[0]) == 0) /* Status */
  {
    _replyStatus();
  }
  else if (strcmp(_doc[msgKW[2]], msgInfo[1]) == 0) /* Query*/
  {
    _replyQuery();
  }
  else if (strcmp(_doc[msgKW[2]], msgInfo[2]) == 0) /* boot Parameters */
  {
    _update_bootP(_doc);
  }
  else if (strcmp(_doc[msgKW[2]], msgInfo[7]) == 0) /* Ping back */
  {
    sendMSG(msgTypes[1], msgInfo[7]);
  }
}

void _update_bootP(JsonDocument &_doc)
{
  Err_Protect = _doc["err_p"];
  DualSW = _doc["dub_sw"];
  AutoOff = _doc["t_out"];
  AutoOff_duration = _doc["t_out_d"];
  bootTime = _doc["boot_t"].as<time_t>();
  Lockdown = _doc["Lockdown"];
  btype_2 = _doc["btype_2"]; /* Button type for external input only. This part is not solved yet */

  getP_OK = true;
}
void _ask_bootP()
{
  sendMSG(msgTypes[1], msgInfo[2]); /* calling for remote parameters */
}
void request_remoteParameters(uint8_t _waitDuration = 15)
{
  long last_req = millis();
  _ask_bootP();
  while (millis() < _waitDuration * 1000 && getP_OK == false) /* Wait to get parameters */
  {
    if (millis() - last_req > 2000) /* ask again */
    {
      last_req = millis();
      _ask_bootP();
    }
    readSerial();
    delay(50);
  }
}
void reset_fail_load_parameters()
{
  if (getP_OK == false && millis() > MIN2RESET_BAD_P * 60000UL)
  {
    sendMSG(msgTypes[2], msgErrs[1]);
    delay(1000);
    resetFunc();
  }
}
void postBoot_err_notification()
{
  sendMSG(msgTypes[1], msgInfo[3]);
  struct tm *tm = localtime(&bootTime);

  if (tm->tm_year == 70)
  {
    sendMSG(msgTypes[2], msgInfo[3], "NTP");
  }
  if (getP_OK == false)
  {
    sendMSG(msgTypes[2], msgInfo[3], "Parameters");
  }
}

void Serial_CB(JsonDocument &_doc)
{
  if (strcmp(_doc[msgKW[1]], msgTypes[0]) == 0) /* Got Actions */
  {
    _Actions_cb(_doc[msgKW[2]]);
  }
  else if (strcmp(_doc[msgKW[1]], msgTypes[1]) == 0) /* info */
  {
    _Infos_cb(_doc);
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
      // #if DEBUG_MODE
      //       Serial.print("\nGot msg: ");
      //       serializeJson(doc, Serial);
      //       Serial.flush();
      // #endif
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

// ~~~~~~ Handling Inputs & Outputs ~~~~~~~
void allRelays_Off()
{
  digitalWrite(REL_UP_PIN, !RELAY_ON);
  digitalWrite(REL_DOWN_PIN, !RELAY_ON);
  delay(20);
}
void start_output_gpios()
{
  pinMode(REL_UP_PIN, OUTPUT);
  pinMode(REL_DOWN_PIN, OUTPUT);
  allRelays_Off();
}
void start_buttSW()
{
  buttSwitch.pin0 = SW_UP_PIN;
  buttSwitch.pin1 = SW_DOWN_PIN;
  buttSwitch.buttonType = 2;
  buttSwitch.start();

  if (DualSW)
  {
    static buttonPresses buttSwitchExt;
    buttSwitchEXT[0] = &buttSwitchExt;
    buttSwitchEXT[0]->pin0 = SW2_UP_PIN;
    buttSwitchEXT[0]->pin1 = SW2_DOWN_PIN;
    buttSwitchEXT[0]->buttonType = btype_2;
    buttSwitchEXT[0]->start();
  }
}

uint8_t getWin_state()
{
  bool relup = digitalRead(REL_UP_PIN);
  bool reldown = digitalRead(REL_DOWN_PIN);

  if (relup == !RELAY_ON && reldown == !RELAY_ON)
  {
    return WIN_STOP; // value 3
  }
  else if (relup == RELAY_ON && reldown == !RELAY_ON)
  {
    return WIN_UP; // value 1
  }
  else if (relup == !RELAY_ON && reldown == RELAY_ON)
  {
    return WIN_DOWN; // value 2
  }
  else
  {
    return WIN_ERR; // value 0
  }
}
void read_buttSwitch()
{
  uint8_t switchRead = buttSwitch.getValue(); /*  0: no change; 1: up; 2: down; 3: off */
  if (switchRead != 0)
  {
    switch_cb(switchRead, msgInfo[5]);
  }
}

void autoOff_looper()
{
  if (AutoOff && autoOff_clock != 0)
  {
    if (millis() > autoOff_clock + AutoOff_duration * 1000UL)
    {
      switch_cb(WIN_STOP, msgAct[5]);
    }
  }
}
bool _makeSwitch(uint8_t state)
{
  if (getWin_state() != state) /* Not already in that state */
  {
    allRelays_Off();
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

    if (AutoOff)
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
  if (Err_Protect)
  {
    if (digitalRead(REL_UP_PIN) == RELAY_ON && digitalRead(REL_DOWN_PIN) == RELAY_ON)
    {
      switch_cb(WIN_STOP, msgInfo[4]);
    }
    if (digitalRead(SW_UP_PIN) == SW_PRESSED && digitalRead(SW_DOWN_PIN) == SW_PRESSED)
    {
      sendMSG(msgTypes[2], "Buttons");
      delay(100);
      resetFunc();
    }
    if (DualSW)
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

void update_lockdown_state(bool _state)
{
  if (_state)
  {
    switch_cb(WIN_DOWN, msgAct[6]);
    sendMSG(msgTypes[1], msgAct[6]);
  }
  else
  {
    sendMSG(msgTypes[1], msgAct[7]);
  }
  lockdown_state = _state;
}
bool checkLockdown()
{
  if (Lockdown)
  {
    return lockdown_state;
  }
  else
  {
    return 0;
  }
}

void setup()
{
  start_output_gpios();
  start_buttSW();
  Serial.begin(9600);
  request_remoteParameters();
  postBoot_err_notification();
}
void loop()
{
  read_buttSwitch();
  // errorProtection(); /* Avoid Simulatnious UP&DOWN */
  readSerial();
  autoOff_looper();
  reset_fail_load_parameters();
}