bool doubleSW = false;
bool Lockdown = false;
bool useAutoOff = false;
uint8_t btype_2 = 2;
uint8_t autoOff_time = 120;
// static int sketch_JSON_Psize = 256; /* Pass JSON size for Flash Parameter*/

void update_vars(JsonDocument &DOC)
{
  btype_2 = DOC["btype_2"];
  Lockdown = DOC["Lockdown"];
  doubleSW = DOC["doubleSW"];
  useAutoOff = DOC["useAutoOff"];
  autoOff_time = DOC["autoOff_time"];
}
void read_flashParameter()
{
  StaticJsonDocument <256> sketchJSON;
  char sketch_defs[] = "{\"useAutoOff\":false,\"autoOff_time\":60,\"doubleSW\":false,\
                          \"btype_2\":2, \"Lockdown\":false}";

  iot.read_fPars(iot.sketch_paramfile, sketchJSON, sketch_defs); /* Read sketch defs */
  update_vars(sketchJSON);
  sketchJSON.clear();
}
