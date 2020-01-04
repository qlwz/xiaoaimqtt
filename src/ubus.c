
#include "ubus.h"
#include "cJSON.h"

unsigned char lastVolume = 0;     // 最后音量
unsigned char lastStatus = 0;     // 最后状态
unsigned char lastPlayerType = 0; // 最后播放类型
char lastContext[2046];           // 最后内容
char statusBuff[2046];

unsigned char playerPlayOperation(char *oper)
{
    unsigned char result = -1;
    sprintf(statusBuff, "ubus call mediaplayer player_play_operation {\\\"action\\\":\\\"%s\\\"}", oper);
    shellcmd(statusBuff, statusBuff, sizeof(statusBuff));
    cJSON *root = cJSON_Parse(statusBuff);
    if (root)
    {
        cJSON *code = cJSON_GetObjectItem(root, "code");
        if (code && code->type == cJSON_Number)
        {
            result = code->valueint;
        }
    }
    cJSON_Delete(root);
    return result;
}

unsigned char getStatus()
{
    unsigned char flag = 0;
    //memset(statusBuff, 0, sizeof(statusBuff));
    shellcmd("ubus call mediaplayer player_get_context", statusBuff, sizeof(statusBuff));
    cJSON *root = cJSON_Parse(statusBuff);
    if (root)
    {
        cJSON *info = cJSON_GetObjectItem(root, "info");
        if (info)
        {
            if (strncmp((const char *)lastContext, info->valuestring, strlen(info->valuestring)) != 0)
            {
                strncpy((char *)lastContext, info->valuestring, strlen(info->valuestring));
                flag |= 1;
                printf("Context:%s\n", lastContext);
            }

            cJSON *infoRoot = cJSON_Parse(info->valuestring);
            if (infoRoot)
            {
                cJSON *status = cJSON_GetObjectItem(infoRoot, "device_player_status");
                if (status)
                {
                    cJSON *player_type = cJSON_GetObjectItem(status, "player_type");
                    if (player_type)
                    {
                        if (lastPlayerType != player_type->valueint)
                        {
                            flag |= 2;
                            lastPlayerType = player_type->valueint;
                            printf("PlayerType:%d\n", lastPlayerType);
                        }
                    }
                    cJSON *player_volume = cJSON_GetObjectItem(status, "player_volume");
                    if (player_volume)
                    {
                        if (lastVolume != player_volume->valueint)
                        {
                            flag |= 4;
                            lastVolume = player_volume->valueint;
                            printf("Volume:%d\n", lastVolume);
                        }
                    }
                    cJSON *player_status = cJSON_GetObjectItem(status, "player_status");
                    if (player_status)
                    {
                        if (lastStatus != player_status->valueint)
                        {
                            flag |= 8;
                            lastStatus = player_status->valueint;
                            printf("Status:%d\n", lastStatus);
                        }
                    }
                }
            }
        }
    }
    cJSON_Delete(root);
    return flag;
}

int shellcmd(char *cmd, char *buff, int size)
{
    char temp[256];
    FILE *fp = NULL;
    int offset = 0;
    int len;

    fp = popen(cmd, "r");
    if (fp == NULL)
    {
        return -1;
    }
    while (fgets(temp, sizeof(temp), fp) != NULL)
    {
        len = strlen(temp);
        if (offset + len < size)
        {
            strcpy(buff + offset, temp);
            offset += len;
        }
        else
        {
            buff[offset] = 0;
            break;
        }
    }
    if (buff[offset - 1] == '\n')
    {
        offset--;
        buff[offset] = 0;
    }
    if (buff[offset - 1] == '\r')
    {
        offset--;
        buff[offset] = 0;
    }
    pclose(fp);
    return offset;
}

void shellcmdNoResult(char *cmd)
{
    printf("cmd:%s\n", cmd);
    system(cmd);
}