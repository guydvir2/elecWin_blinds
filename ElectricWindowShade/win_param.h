#include <Arduino.h>
#include <myIOT2.h>
#include <ArduinoJson.h>

bool readfile_ok = false;
char *sketch_paramfile = "/sketch_param.json";
StaticJsonDocument<JSON_SIZE_IOT> paramJSON;
StaticJsonDocument<JSON_SIZE_SKETCH> sketchJSON;

extern myIOT2 iot;

void update_vars(JsonDocument &DOC)
{
  inputUpPin = DOC["inputUpPin"];
  inputDownPin = DOC["inputDownPin"];
  outputUpPin = DOC["outputUpPin"];
  outputDownPin = DOC["outputDownPin"];
  inputUpExtPin = DOC["inputUpExtPin"];
  inputDownExtPin = DOC["inputDownExtPin"];
  useExtInput = DOC["useExtInput"];
  useAutoRelayOFF = DOC["useAutoRelayOFF"];
  AutoRelayOff_timeout = DOC["AutoRelayOff_timeout"];
}
void startRead_parameters()
{
  String sketch_defs = "{\"useExtInput\":false,\"useAutoRelayOFF\":false,\"inputUpPin\":4,\"inputDownPin\":5,\
                        \"outputUpPin\":14,\"outputDownPin\":12,\"inputUpExtPin\":0,\"inputDownExtPin\":2,\"AutoRelayOff_timeout\":60}";

  String myIOT_defs = "{\"useSerial\":true,\"useWDT\":false,\"useOTA\":true,\"useResetKeeper\" : false,\
                        \"useFailNTP\" : true,\"useDebugLog\" : true,\"useNetworkReset\":false, \"deviceTopic\" : \"myWindow\",\
                        \"groupTopic\" : \"Windows\",\"prefixTopic\" : \"myHome\",\"debug_level\":0,\"noNetwork_reset\":1}";

  bool a = iot.read_fPars(sketch_paramfile, sketch_defs, sketchJSON);
  bool b = iot.read_fPars(iot.myIOT_paramfile, myIOT_defs, paramJSON);
  readfile_ok = a && b;
  
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
