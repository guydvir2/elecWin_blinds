#include <Arduino.h>
// char *sketch_paramfile = "/sketch_param.json";

extern myIOT2 iot;

bool doubleSW = false;
bool Lockdown = false;
bool useAutoOff = false;
bool err_protect = false;
uint8_t btype_2 = 2;
uint8_t autoOff_time = 180;
int sketch_JSON_Psize = 1250; /* Pass JSON size for Flash Parameter*/

void update_vars(JsonDocument &DOC)
{ 
  btype_2 = DOC["btype_2"];
  Lockdown = DOC["Lockdown"];
  useAutoOff = DOC["useAutoOff"];
  err_protect = DOC["err_protect"];
  autoOff_time = DOC["autoOff_time"];
}
void startRead_parameters()
{
  DynamicJsonDocument sketchJSON(sketch_JSON_Psize);

  char myIOT_defs[] = "{\"useSerial\":true,\"useWDT\":false,\"useOTA\":true,\"useResetKeeper\" : false,\"useBootClockLog\" : false,\
                        \"useDebugLog\" : true,\"useNetworkReset\":false, \"deviceTopic\" : \"myWindow\",\
                        \"groupTopic\" : \"Windows\",\"prefixTopic\" : \"myHome\",\"debug_level\":0,\"noNetwork_reset\":5}";

  iot.read_fPars(iot.sketch_paramfile, sketchJSON, myIOT_defs); /* Read sketch defs */
  // serializeJson(sketchJSON, Serial);
  // Serial.flush();
  update_vars(sketchJSON);
  sketchJSON.clear();
  // delete sketchJSON;
}
