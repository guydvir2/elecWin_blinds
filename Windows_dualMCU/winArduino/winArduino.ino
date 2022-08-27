#include <Arduino.h>
#include <time.h>
#include <buttonPresses.h>
#include "constants.h"
#include "SerialComm.h"
#include "Serial_bootParameters.h"

buttonPresses *buttSwitches[] = {nullptr, nullptr, nullptr};

bool lockdown_state = false; /* Flag, lockdown command */
unsigned long autoOff_clock[2] = {0, 0};

// ~~~~~~~~~~~~~ Callbacks ~~~~~~~~~~~~~~~
void _replyStatus(uint8_t i)
{
  sendMSG(msgTypes[1], msgInfo[0], msgAct[_getRelay_state(i)], i);
}
void _replyQuery()
{
  char t[250];
  char clk2[25];
  uint8_t day_light = 3;
  bootTime = 1641219475;
  struct tm *tm = localtime(&bootTime);
  if (tm->tm_mon >= 10 && tm->tm_mon < 4)
  {
    day_light = 2;
  }

  sprintf(clk2, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour + day_light, tm->tm_min, tm->tm_sec);

  sprintf(t, "ver[%s], MCU[%s], DualSW[%s], BootP[%s], boot[%s],Auto_off[%s %dsec], Lockdown[%s]",
          VER, MCU_TYPE, DualSW ? "YES" : "NO", getP_OK ? "OK" : "FAIL", clk2, AutoOff ? "YES" : "NO", AutoOff_duration, Lockdown ? "YES" : "NO");
  sendMSG(msgTypes[1], msgInfo[1], t);
}
void _Actions_cb(const char *KW2, uint8_t i)
{
  if (strcmp(KW2, msgAct[WIN_UP]) == 0) /* Window UP */
  {
    switch_cb(WIN_UP, msgInfo[6], i);
  }
  else if (strcmp(KW2, msgAct[WIN_DOWN]) == 0) /* Window DOWN */
  {
    switch_cb(WIN_DOWN, msgInfo[6], i);
  }
  else if (strcmp(KW2, msgAct[WIN_STOP]) == 0) /* Window OFF */
  {
    switch_cb(WIN_STOP, msgInfo[6], i);
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
void _Infos_cb(JsonDocument &_doc, uint8_t i)
{
  if (strcmp(_doc[msgKW[2]], msgInfo[0]) == 0) /* Status */
  {
    _replyStatus(i);
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
    sendMSG(msgTypes[1], msgInfo[7], i);
  }
}
void Serial_CB(JsonDocument &_doc)
{
  if (strcmp(_doc[msgKW[1]], msgTypes[0]) == 0) /* Got Actions */
  {
    _Actions_cb(_doc[msgKW[2]], _doc[msgKW[4]]);
  }
  else if (strcmp(_doc[msgKW[1]], msgTypes[1]) == 0) /* info */
  {
    _Infos_cb(_doc, _doc[msgKW[4]]);
  }
}
void switch_cb(uint8_t value, char *src, uint8_t i)
{
  if (!_check_Lockdown_state())
  {
    if (_makeSwitch(value, i))
    {
      sendMSG(msgTypes[0], msgAct[value], src, i);
    }
  }
  else /* in LOCKDOWN mode */
  {
    if (_makeSwitch(WIN_STOP, i)) /* Allow AutoOff*/
    {
      sendMSG(msgTypes[0], msgAct[WIN_STOP], src, i);
    }
  }
}

// ~~~~~~ Handling Inputs & Outputs ~~~~~~~
void start_output_gpios()
{
  for (uint8_t i = 0; i < numWindows; i++)
  {
    pinMode(rel_up_pins[i], OUTPUT);
    pinMode(rel_down_pins[i], OUTPUT);
  }
  allRelays_Off();
}
void start_buttSW()
{
  for (uint8_t i = 0; i < numWindows; i++)
  {
    buttSwitches[i] = new buttonPresses;
    buttSwitches[i]->pin0 = sw_up_pins[i];
    buttSwitches[i]->pin1 = sw_down_pins[i];
    buttSwitches[i]->buttonType = 2;
    buttSwitches[i]->start();
  }
  if (DualSW)
  {
    buttSwitches[2] = new buttonPresses;
    buttSwitches[2]->pin0 = SW3_UP_PIN;
    buttSwitches[2]->pin1 = SW3_DOWN_PIN;
    buttSwitches[2]->buttonType = 2;
    buttSwitches[2]->start();
  }
}
void allRelays_Off()
{
  WIN_OFF(0);
  WIN_OFF(1);
  delay(20);
}
uint8_t _getRelay_state(uint8_t i)
{
  bool relup = false;
  bool reldown = false;

  if (i < numWindows)
  {
    relup = digitalRead(rel_up_pins[i]);
    reldown = digitalRead(rel_down_pins[i]);
  }
  else
  {
    return WIN_ERR;
  }

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
bool _makeSwitch(uint8_t state, uint8_t i)
{
  if (_getRelay_state(i) != state) /* Not already in that state */
  {
    switch (state)
    {
    case WIN_STOP:
      WIN_OFF(i);
      break;
    case WIN_UP:
      WIN_UP(i);
      break;
    case WIN_DOWN:
      WIN_DOWN(i);
      break;
    default:
      break;
    }
    start_autoOff_timeout(state, i);
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
    for (uint8_t i = 0; i < numWindows; i++)
    {
      switch_cb(WIN_DOWN, msgAct[6], i);
      sendMSG(msgTypes[1], msgAct[6], "", i);
    }
  }
  else
  {
    sendMSG(msgTypes[1], msgAct[7]);
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
  uint8_t switchRead;
  for (uint8_t i = 0; i < numWindows; i++)
  {
    switchRead = buttSwitches[i]->read(); /*  0: stop; 1: up; 2: down; 3:err ; 4: nochange*/
    if (switchRead < 3)
    {
      switch_cb(switchRead, msgInfo[5], i);
      return;
    }
  }
  if (DualSW)
  {
    switchRead = buttSwitches[2]->read(); /*  0: no change; 1: up; 2: down; 3: off */
    if (switchRead < 3)
    {
      switch_cb(switchRead, msgInfo[8], 0); /* external input only for windows 0*/
      return;
    }
  }
}
void autoOff_looper()
{
  if (AutoOff)
  {
    for (uint8_t i = 0; i < numWindows; i++)
    {
      if (autoOff_clock[i] != 0)
      {
        if (millis() > autoOff_clock[i] + AutoOff_duration * 1000UL)
        {
          switch_cb(WIN_STOP, msgAct[5], i);
        }
      }
    }
  }
}
void start_autoOff_timeout(uint8_t state, uint8_t i)
{
  if (AutoOff)
  {
    if (state != WIN_STOP)
    {
      autoOff_clock[i] = millis();
    }
    else
    {
      autoOff_clock[i] = 0;
    }
  }
}
void setup()
{
  start_output_gpios();
  Serial.begin(9600);
  request_remoteParameters();
  start_buttSW();
  postBoot_err_notification();
}
void loop()
{
  readSwitch_looper();
  readSerial();
  autoOff_looper();
  reset_fail_load_parameters();
}