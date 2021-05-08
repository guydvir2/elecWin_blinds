#include <myIOT2.h>
#include <ArduinoJson.h>


bool readfile_ok = false;
char *sketch_paramfile = "/sketch_param.json";
StaticJsonDocument<JSON_SIZE_IOT> paramJSON;
StaticJsonDocument<JSON_SIZE_SKETCH> sketchJSON;

extern myIOT2 iot;

void update_vars(JsonDocument &DOC)
{
  outputUpPin = DOC["outputUpPin"];
  outputDownPin = DOC["outputDownPin"];
  relayUpPin = DOC["relayUpPin"];
  relayDownPin = DOC["relayDownPin"];
  useAutoOff = DOC["useAutoOff"];
  autoOff_time = DOC["autoOff_time"];
}
void startRead_parameters()
{
  String sketch_defs = "{\"outputUpPin\":14,\"outputDownPin\":12,\"relayUpPin\":4,\"relayDownPin\":5,\"useAutoOff\":false,\"autoOff_time\":60}";

  String myIOT_defs = "{\"useSerial\":true,\"useWDT\":false,\"useOTA\":true,\"useResetKeeper\" : false,\"useBootClockLog\" : false,\
                        \"useFailNTP\" : true,\"useDebugLog\" : true,\"useNetworkReset\":false, \"deviceTopic\" : \"myWindow\",\
                        \"groupTopic\" : \"Windows\",\"prefixTopic\" : \"myHome\",\"debug_level\":0,\"noNetwork_reset\":1}";

  bool a = iot.read_fPars(sketch_paramfile, sketch_defs, sketchJSON);
  bool b = iot.read_fPars(iot.myIOT_paramfile, myIOT_defs, paramJSON);
  readfile_ok = a && b;
  // serializeJsonPretty(sketchJSON, Serial);
  // serializeJsonPretty(paramJSON, Serial);
  // Serial.flush();
  update_vars(sketchJSON);
}
void endRead_parameters()
{
  if (!readfile_ok)
  {
    iot.pub_log("Error read Parameters from file. Defaults values loaded.");
  }
  paramJSON.clear();
  sketchJSON.clear();
}
