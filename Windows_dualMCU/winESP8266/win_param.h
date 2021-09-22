bool readfile_ok = false;
bool useAutoOff = false;
bool doubleSW = false;
bool err_protect = false;
bool Lockdown = false;
uint8_t autoOff_time = 200;
uint8_t btype_2 = 2;

char *sketch_paramfile = "/sketch_param.json";
StaticJsonDocument<JSON_SIZE_IOT> paramJSON;
StaticJsonDocument<JSON_SIZE_SKETCH> sketchJSON;

extern myIOT2 iot;

void update_vars(JsonDocument &DOC)
{
  useAutoOff = DOC["useAutoOff"];
  autoOff_time = DOC["autoOff_time"];
  err_protect = DOC["err_protect"];
  doubleSW = DOC["doubleSW"];
  btype_2 = DOC["btype_2"];
  Lockdown = DOC["Lockdown"];
}
void startRead_parameters()
{
  String sketch_defs = "{\"useAutoOff\":false,\"autoOff_time\":60,\"doubleSW\":false,\"err_protect\":false,\"btype_2\":2, \"Lockdown\":false}";

  String myIOT_defs = "{\"useSerial\":true,\"useWDT\":false,\"useOTA\":true,\"useResetKeeper\" : false,\"useBootClockLog\" : false,\
                        \"useDebugLog\" : true,\"useNetworkReset\":false, \"deviceTopic\" : \"myWindow\",\
                        \"groupTopic\" : \"Windows\",\"prefixTopic\" : \"myHome\",\"debug_level\":0,\"noNetwork_reset\":5}";

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
