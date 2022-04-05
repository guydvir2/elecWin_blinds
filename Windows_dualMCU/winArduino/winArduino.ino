#include <TimeLib.h>
#include <Arduino.h>
#include <buttonPresses.h>
#include <mySerialMSG.h>
#include "constants.h"

buttonPresses buttSwitch;
buttonPresses *buttSwitchEXT[] = {nullptr};
mySerialMSG SerialComm(DEV_NAME, COMM_SERIAL);

#include "Serial_bootParameters.h"

bool lockdown_state = false;
unsigned long autoOff_clock = 0;

// ~~~~~~~~~~~~~ Callbacks ~~~~~~~~~~~~~~~
void _replyStatus()
{
  SerialComm.sendMsg(DEV_NAME, msgTypes[1], msgInfo[0], msgAct[_getRelay_state()]);
}
void _replyQuery()
{
  char t[250];
  char clk2[25];
  uint8_t day_light = 3;
  if (month(bootTime) > 9 && month(bootTime) < 4)
  {
    day_light = 2;
  }
  sprintf(clk2, "%04d-%02d-%02d %02d:%02d:%02d", year(bootTime), month(bootTime), day(bootTime), hour(bootTime) + day_light, minute(bootTime), second(bootTime));
  sprintf(t, "ver[%s], MCU[%s], DualSW[%s], BootP[%s], boot[%s],Auto_off[%s %dsec], Lockdown[%s]",
          VER, MCU_TYPE, DualSW ? "YES" : "NO", getP_OK ? "OK" : "FAIL", clk2, AutoOff ? "YES" : "NO", AutoOff_duration, Lockdown ? "YES" : "NO");
  SerialComm.sendMsg(DEV_NAME, msgTypes[1], msgInfo[1], t);
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
    SerialComm.sendMsg(DEV_NAME, msgTypes[1], msgInfo[7]);
  }
}
void incomeMSG_cb(JsonDocument &_doc)
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
void switch_cb(uint8_t value, char *src)
{
  if (!_check_Lockdown_state())
  {
    if (_makeSwitch(value))
    {
      SerialComm.sendMsg(DEV_NAME, msgTypes[0], msgAct[value], src);
    }
  }
  else /* in LOCKDOWN mode */
  {
    if (_makeSwitch(WIN_STOP)) /* Allow AutoOff*/
    {
      SerialComm.sendMsg(DEV_NAME, msgTypes[0], msgAct[WIN_STOP], src);
    }
  }
}

void init_serialMSG()
{
  COMM_SERIAL.begin(9600);

  SerialComm.KW[0] = msgKW[0];
  SerialComm.KW[1] = msgKW[1];
  SerialComm.KW[2] = msgKW[2];
  SerialComm.KW[3] = msgKW[3];

  SerialComm.usePings = true;
  SerialComm.start(incomeMSG_cb);
}

// ~~~~~~ Handling Inputs & Outputs ~~~~~~~
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
void allRelays_Off()
{
  digitalWrite(REL_UP_PIN, !RELAY_ON);
  digitalWrite(REL_DOWN_PIN, !RELAY_ON);
  delay(20);
}
uint8_t _getRelay_state()
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
bool _makeSwitch(uint8_t state)
{
  if (_getRelay_state() != state) /* Not already in that state */
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

void update_lockdown_state(bool _state)
{
  if (_state)
  {
    switch_cb(WIN_DOWN, msgAct[6]);
    SerialComm.sendMsg(DEV_NAME, msgTypes[1], msgAct[6]);
  }
  else
  {
    SerialComm.sendMsg(DEV_NAME, msgTypes[1], msgAct[7]);
  }
  lockdown_state = _state;
}
bool _check_Lockdown_state()
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
void readSwitch_looper()
{
  uint8_t switchRead = buttSwitch.read(); /*  0: stop; 1: up; 2: down; 3:err ; 4: nochange*/
  if (switchRead < 3)
  {
    switch_cb(switchRead, msgInfo[5]);
  }
  if (DualSW)
  {
    uint8_t switchRead = buttSwitchEXT[0]->read(); /*  0: no change; 1: up; 2: down; 3: off */
    if (switchRead != 0)
    {
      switch_cb(switchRead, msgInfo[8]);
    }
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

void setup()
{
  start_output_gpios();
  init_serialMSG();           /* Start SerialComm between MCUs*/
  request_remoteParameters(); /* Request Flash Parameter from ESP8266 CPU via SerialComm*/
  start_buttSW();             /* init input Button instances */
  postBoot_err_notification();
}
void loop()
{
  readSwitch_looper();
  SerialComm.loop();
  autoOff_looper();

  reset_fail_load_parameters();
}