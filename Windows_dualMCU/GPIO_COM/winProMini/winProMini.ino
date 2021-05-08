#define REL_DOWN 2         /* OUTUPT to relay device */
#define REL_UP 3           /* OUTUPT to relay device */
#define SW_UP 4            /* Switch INPUT to ProMini */
#define SW_DOWN 5          /* Switch INPUT to ProMini */
#define ESP_UP 11          /* ESP INPUT to ProMini */
#define ESP_DOWN 12        /* ESP INPUT to ProMini */
#define REL_UP_TO_ESP 13   /* OUTPUT to ESP - Relay state*/
#define REL_DOWN_TO_ESP 10 /* OUTPUT to ESP - Relay state*/

#define SW_PRESSED LOW
#define RELAY_ON LOW

#define VER "ProMini_v0.2"

const byte WIN_STOP = 0;
const byte WIN_UP = 1;
const byte WIN_DOWN = 2;

const byte change_dir_delay = 20; //ms
const byte debounce_delay = 20;   //ms
const byte loop_delay = 10;       //ms

bool swUp_lastState = false;
bool swDown_lastState = false;
bool espUp_lastState = false;
bool espDown_lastState = false;

void start_gpio()
{
  pinMode(SW_UP, INPUT_PULLUP);
  pinMode(SW_DOWN, INPUT_PULLUP);
  pinMode(ESP_UP, INPUT_PULLUP);
  pinMode(ESP_DOWN, INPUT_PULLUP);

  pinMode(REL_UP, OUTPUT);
  pinMode(REL_DOWN, OUTPUT);
  pinMode(REL_UP_TO_ESP, OUTPUT);
  pinMode(REL_DOWN_TO_ESP, OUTPUT);

  allOff();
}
byte check_current_relState()
{
  bool relup = digitalRead(REL_UP);
  bool reldown = digitalRead(REL_DOWN);

  if (relup && reldown == !RELAY_ON)
  {
    return WIN_STOP;
  }
  else if (relup == RELAY_ON)
  {
    return WIN_UP;
  }
  else
  {
    return WIN_DOWN;
  }
}
void readInput(int inPin, int outPin, bool &lastState)
{
  bool state = digitalRead(inPin);
  if (state != lastState)
  {
    delay(debounce_delay);
    if (digitalRead(inPin) == state)
    {
      byte relays_state = check_current_relState();
      if (state == SW_PRESSED)
      {
        Serial.print("pin ");
        Serial.print(inPin);
        Serial.println(" pressed");
        if (inPin == SW_UP || inPin == ESP_UP)
        {
          if (relays_state != WIN_UP)
          {
            makeSwitch(WIN_UP);
          }
          else
          {
            Serial.println("ignored");
          }
          lastState = state;
        }
        else if (inPin == SW_DOWN || inPin == ESP_DOWN)
        {
          if (relays_state != WIN_DOWN)
          {
            makeSwitch(WIN_DOWN);
          }
          else
          {
            Serial.println("ignored");
          }
          lastState = state;
        }
        else
        {
          if (relays_state != WIN_STOP)
          {
            makeSwitch(WIN_STOP);
          }
          else
          {
            Serial.println("ignored");
          }
          lastState = !SW_PRESSED;
        }
      }
      else
      {
        Serial.print("pin ");
        Serial.print(inPin);
        Serial.println(" released");
        makeSwitch(WIN_STOP);
        lastState = !SW_PRESSED;
      }
    }
  }
}
void allOff()
{
  digitalWrite(REL_UP, !RELAY_ON);
  digitalWrite(REL_DOWN, !RELAY_ON);
  digitalWrite(REL_UP_TO_ESP, !RELAY_ON);
  digitalWrite(REL_DOWN_TO_ESP, !RELAY_ON);
  delay(change_dir_delay);
}
void makeSwitch(byte state)
{
  if (state == WIN_STOP) /* Stop */
  {
    allOff();
  }
  else if (state == WIN_UP) /* Up */
  {
    allOff();
    digitalWrite(REL_UP, RELAY_ON);
    digitalWrite(REL_UP_TO_ESP, RELAY_ON);
    Serial.println("Switch Up");
  }
  else if (state == WIN_DOWN) /* DOWN */
  {
    allOff();
    digitalWrite(REL_DOWN, RELAY_ON);
    digitalWrite(REL_DOWN_TO_ESP, RELAY_ON);
    Serial.println("Switch Down");
  }
  else
  {
    allOff();
    Serial.println("off due error");
  }
}
void errorProtection()
{
  if (digitalRead(REL_UP) == RELAY_ON && digitalRead(REL_DOWN) == RELAY_ON)
  {
    makeSwitch(WIN_STOP);
    Serial.println("Protection error - Relays");
  }
  if (digitalRead(SW_UP) == SW_PRESSED && digitalRead(SW_DOWN) == SW_PRESSED)
  {
    Serial.println("Protection error - Switches");
  }
  if (digitalRead(ESP_UP) == SW_PRESSED && digitalRead(ESP_DOWN) == SW_PRESSED)
  {
    Serial.println("Protection error - ESP");
  }
}
void read_allInputs()
{
  readInput(SW_UP, REL_UP, swUp_lastState);
  readInput(SW_DOWN, REL_DOWN, swDown_lastState);
  readInput(ESP_UP, REL_UP, espUp_lastState);
  readInput(ESP_DOWN, REL_DOWN, espDown_lastState);
}

void setup()
{
  start_gpio();
  Serial.begin(115200);
}
void loop()
{
  read_allInputs();
  errorProtection();
  delay(loop_delay);
}
