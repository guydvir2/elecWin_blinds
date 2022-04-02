extern myIOT2 iot;

bool useAutoOff = false;
bool doubleSW = false;
bool err_protect = false;
bool Lockdown = false;
int sketch_JSON_Psize = 256;
uint8_t autoOff_time = 200;
uint8_t btype_2 = 2;

void send_boot_parameters()
{
  StaticJsonDocument<JSON_SERIAL_SIZE> doc;

  doc[msgKW[0]] = DEV_NAME;
  doc[msgKW[1]] = msgTypes[1];
  doc[msgKW[2]] = msgInfo[2];

  /* Following parameters will update from flash */
  doc["err_p"] = err_protect;
  doc["dub_sw"] = doubleSW;
  doc["t_out"] = useAutoOff;
  doc["t_out_d"] = autoOff_time;
  doc["boot_t"] = (long)iot.now();
  doc["btype_2"] = btype_2;
  doc["Lockdown"] = Lockdown;

  serializeJson(doc, Serial);
}
void update_vars(JsonDocument &DOC)
{
  useAutoOff = DOC["useAutoOff"];
  autoOff_time = DOC["autoOff_time"];
  err_protect = DOC["err_protect"];
  doubleSW = DOC["doubleSW"];
  btype_2 = DOC["btype_2"];
  Lockdown = DOC["Lockdown"];
  sketch_JSON_Psize = DOC["sketch_JSON_Psize"];
}
void readFlash_parameters()
{
  DynamicJsonDocument sketchJSON(sketch_JSON_Psize);

  char sketch_defs[] = "{\"useAutoOff\":false,\"autoOff_time\":60,\"doubleSW\":false,\
                          \"err_protect\":false,\"btype_2\":2, \"Lockdown\":false,\
                          \"sketch_JSON_Psize\":256}";

  bool a = iot.read_fPars(iot.sketch_paramfile, sketchJSON, sketch_defs); /* Read sketch defs */
  // serializeJsonPretty(sketchJSON, Serial);
  // Serial.flush();
  update_vars(sketchJSON);
  sketchJSON.clear();
}
