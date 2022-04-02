MQTT_msg lockDown_topic; /* ExtTopic*/

void addiotnalMQTT(char *incoming_msg)
{
    char msg[100];

    if (strcmp(incoming_msg, "status") == 0)
    {
        SerialComm.sendMsg(DEV_NAME,msgTypes[1], msgInfo[0]);
    }
    else if (strcmp(incoming_msg, "up") == 0)
    {
        SerialComm.sendMsg(DEV_NAME,msgTypes[0], msgAct[WIN_UP], msgInfo[6]);
    }
    else if (strcmp(incoming_msg, "down") == 0)
    {
        SerialComm.sendMsg(DEV_NAME,msgTypes[0], msgAct[WIN_DOWN], msgInfo[6]);
    }
    else if (strcmp(incoming_msg, "off") == 0)
    {
        SerialComm.sendMsg(DEV_NAME,msgTypes[0], msgAct[WIN_STOP], msgInfo[6]);
    }
    else if (strcmp(incoming_msg, "ver2") == 0)
    {
        sprintf(msg, "ver2:[%s], [DualMCU], [Serial-Comm], AutoOff[%d], AutoOff_time[%d],Lockdown[%d]", VER, useAutoOff, autoOff_time, Lockdown);
        iot.pub_msg(msg);
    }
    else if (strcmp(incoming_msg, "help2") == 0)
    {
        sprintf(msg, "Help: Commands #3 - [up, down, off, query, reset_MCU, show_flash_param, help2]");
        iot.pub_msg(msg);
    }
    else if (strcmp(incoming_msg, "query") == 0)
    {
        SerialComm.sendMsg(DEV_NAME,msgTypes[1], msgInfo[1]);
    }
    else if (strcmp(incoming_msg, "reset_MCU") == 0)
    {
        sprintf(msg, "Reset: sent to MCU");
        iot.pub_msg(msg);
        SerialComm.sendMsg(DEV_NAME,msgTypes[0], msgAct[4]);
    }
    else if (strcmp(incoming_msg, "ping_MCU") == 0)
    {
        sprintf(msg, "[Ping]: sent to MCU");
        iot.pub_msg(msg);
        SerialComm.sendMsg(DEV_NAME,msgTypes[0], msgAct[4]);
    }
}
void startIOTservices()
{
    iot.useFlashP = true;
    iot.useextTopic = true;
    iot.extTopic[0] = ext_topic;
    iot.extTopic_msgArray[0] = &lockDown_topic;
    iot.start_services(addiotnalMQTT);
}
void lockdown_looper()
{
    if (iot.extTopic_newmsg_flag)
    {
        if (strcmp(lockDown_topic.from_topic, ext_topic) == 0)
        {
            if (strcmp(lockDown_topic.msg, "1") == 0)
            {
                if (Lockdown)
                {
                    iot.pub_msg("[Lockdown]: Enabled");
                    SerialComm.sendMsg(DEV_NAME,msgTypes[0], msgAct[6]);
                }
                else
                {
                    iot.pub_msg("[Lockdown]: igonred");
                }
            }
            else if (strcmp(lockDown_topic.msg, "0") == 0)
            {
                if (Lockdown)
                {
                    iot.pub_msg("[Lockdown]: Disabled");
                    iot.pub_noTopic("", ext_topic, true); /* make sure no retained msg is is topic */
                    SerialComm.sendMsg(DEV_NAME,msgTypes[0], msgAct[7]);
                }
            }
            iot.clear_ExtTopicbuff();
        }
    }
}
