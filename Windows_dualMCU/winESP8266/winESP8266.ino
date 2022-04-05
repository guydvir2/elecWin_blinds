#include <myIOT2.h>
#include <mySerialMSG.h>
#include "constants.h"

myIOT2 iot;
mySerialMSG SerialComm(DEV_NAME, COMM_SERIAL);

#include "win_param.h"
#include "myIOT_settings.h"

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
void incomeMSG_cb(JsonDocument &_doc)
{
        char outmsg[150];
        const char *FROM = _doc[msgKW[0]];
        const char *TYPE = _doc[msgKW[1]];
        const char *INFO = _doc[msgKW[2]];
        const char *INFO2 = _doc[msgKW[3]];

        if (strcmp(TYPE, msgTypes[1]) == 0) /* Getting Info */
        {
                if (strcmp(INFO, msgInfo[0]) == 0 || strcmp(INFO, msgInfo[1]) == 0) /* status || query */
                {
                        sprintf(outmsg, "[%s]: %s", INFO, INFO2);
                        iot.pub_msg(outmsg);
                }
                else if (strcmp(INFO, msgInfo[2]) == 0) /* boot_p */
                {
                        send_boot_parameters();
                }
                else if (strcmp(INFO, msgInfo[3]) == 0) /* Boot */
                {
                        sprintf(outmsg, "[%s]: << Power On Boot >>", FROM);
                        iot.pub_log(outmsg);
                }
                else if ((strcmp(INFO, msgAct[6]) == 0 || strcmp(INFO, msgAct[7]) == 0) && Lockdown) /* LOCKDONW ON/ OFF  */
                {
                        sprintf(outmsg, "Locdown: [%s] is %s", FROM,strcmp(INFO, msgAct[6]) == 0?"[locked]":"[released]");
                        iot.pub_log(outmsg);
                }

        }
        else if (strcmp(TYPE, msgTypes[0]) == 0) /* Actions */
        {
                if (strcmp(INFO, msgAct[WIN_UP]) == 0 || strcmp(INFO, msgAct[WIN_DOWN]) == 0 || strcmp(INFO, msgAct[WIN_STOP]) == 0 || strcmp(INFO, msgAct[5]) == 0)
                {
                        sprintf(outmsg, "[%s]: Window switched [%s]", INFO2, INFO);
                        iot.pub_msg(outmsg);
                }
        }
        else if (strcmp(TYPE, msgTypes[2]) == 0 || strcmp(TYPE, msgTypes[4]) == 0) /* Errors  or SerialMSG notifications*/
        {
                sprintf(outmsg, "[%s]: [%s] [%s] [%s]", TYPE, FROM, INFO, INFO2);
                iot.pub_log(outmsg);
        }
}
void send_boot_parameters()
{
        StaticJsonDocument<JSON_SERIAL_SIZE> doc;

        doc[msgKW[0]] = DEV_NAME;
        doc[msgKW[1]] = msgTypes[1];
        doc[msgKW[2]] = msgInfo[2];

        /* Following parameters will update from flash */
        doc["dub_sw"] = doubleSW;
        doc["t_out"] = useAutoOff;
        doc["t_out_d"] = autoOff_time;
        doc["boot_t"] = (long)iot.now();
        doc["btype_2"] = btype_2;
        doc["Lockdown"] = Lockdown;

        serializeJson(doc, COMM_SERIAL);
}

void setup()
{
        read_flashParameter();
        startIOTservices();
        init_serialMSG();
}
void loop()
{
        iot.looper();
        SerialComm.loop();
        lockdown_looper();
}
