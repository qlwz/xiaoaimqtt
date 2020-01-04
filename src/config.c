#include "config.h"
#include "util.h"

char httpPrefix[100];
char xiaoaiSN[20];
char zone[20];
char apiUrl[100];
char asrUrl[100];
char resUrl[100];
unsigned short asrLen;
unsigned short resLen;
unsigned short intervals;
int lastTime;
char asrKeywords[1024];
char resKeywords[1024];

char mqttServer[50];
unsigned short mqttPort;
char mqttUserName[30];
char mqttPassword[50];

//获取当前程序目录
int getCurrentPath(char buf[], char *pFileName)
{
#ifdef WIN32
    GetModuleFileName(NULL, buf, MAX_PATH);
#else
    char pidfile[64];
    sprintf(pidfile, "/proc/%d/cmdline", getpid());

    int fd = open(pidfile, 0x0000, 0);
    int bytes = read(fd, buf, 256);
    close(fd);
    buf[bytes] = '\0';
#endif
    char *p = &buf[strlen(buf)];
    do
    {
        *p = '\0';
        p--;
#ifdef WIN32
    } while ('\\' != *p);
#else
    } while ('/' != *p);
#endif

    p++;
    //配置文件目录
    memcpy(p, pFileName, strlen(pFileName));
    return 0;
}

void updateItemToObject(cJSON *object, const char *string, cJSON *newitem)
{
    if (cJSON_GetObjectItem(object, string))
    {
        cJSON_ReplaceItemInObject(object, string, newitem);
    }
    else
    {
        cJSON_AddItemToObject(object, string, newitem);
    }
}

int readConfig()
{
    char buf[50];
    memset(buf, 0, sizeof(buf));
    getCurrentPath(buf, "xiaoaimqtt.conf");

    char *buffer = readFile(buf);
    if (buffer == NULL)
    {
        printf("load config error\n");
        return -1;
    }
    memset(httpPrefix, 0, sizeof(httpPrefix));
    memset(zone, 0, sizeof(zone));
    memset(apiUrl, 0, sizeof(apiUrl));
    memset(asrUrl, 0, sizeof(asrUrl));
    memset(resUrl, 0, sizeof(resUrl));
    memset(asrKeywords, 0, sizeof(asrKeywords));
    memset(resKeywords, 0, sizeof(resKeywords));

    memset(mqttServer, 0, sizeof(mqttServer));
    memset(mqttUserName, 0, sizeof(mqttUserName));
    memset(mqttPassword, 0, sizeof(mqttPassword));

    asrLen = 0;
    resLen = 0;

    cJSON *root = cJSON_Parse(buffer);
    if (root)
    {

        cJSON *item = cJSON_GetObjectItem(root, "zone");
        if (item)
        {
            strncpy(zone, item->valuestring, strlen(item->valuestring));
        }

        item = cJSON_GetObjectItem(root, "apiUrl");
        if (item)
        {
            strncpy(apiUrl, item->valuestring, strlen(item->valuestring));
        }

        item = cJSON_GetObjectItem(root, "asrUrl");
        if (item)
        {
            asrLen = strlen(item->valuestring);
            strncpy(asrUrl, item->valuestring, asrLen);
        }

        item = cJSON_GetObjectItem(root, "resUrl");
        if (item)
        {
            resLen = strlen(item->valuestring);
            strncpy(resUrl, item->valuestring, resLen);
        }

        item = cJSON_GetObjectItem(root, "intervals");
        if (item)
        {
            if (item->type == cJSON_Number)
            {
                intervals = item->valueint;
            }
            else
            {
                intervals = atoi(item->valuestring);
            }
        }

        item = cJSON_GetObjectItem(root, "lastTime");
        if (item)
        {
            if (item->type == cJSON_Number)
            {
                lastTime = item->valueint;
            }
            else
            {
                lastTime = atoi(item->valuestring);
            }
        }

        item = cJSON_GetObjectItem(root, "asrKeywords");
        if (item)
        {
            if (strlen(item->valuestring) > 0)
            {
                sprintf(asrKeywords, "(%s)", item->valuestring);
            }
            else
            {
                strncpy(asrKeywords, item->valuestring, strlen(item->valuestring));
            }
        }

        item = cJSON_GetObjectItem(root, "resKeywords");
        if (item)
        {
            if (strlen(item->valuestring) > 0)
            {
                sprintf(resKeywords, "(%s)", item->valuestring);
            }
            else
            {
                strncpy(resKeywords, item->valuestring, strlen(item->valuestring));
            }
        }

        item = cJSON_GetObjectItem(root, "mqttServer");
        if (item)
        {
            strncpy(mqttServer, item->valuestring, strlen(item->valuestring));
        }

        item = cJSON_GetObjectItem(root, "mqttPort");
        if (item)
        {
            if (item->type == cJSON_Number)
            {
                mqttPort = item->valueint;
            }
            else
            {
                mqttPort = atoi(item->valuestring);
            }
        }

        item = cJSON_GetObjectItem(root, "mqttUserName");
        if (item)
        {
            strncpy(mqttUserName, item->valuestring, strlen(item->valuestring));
        }

        item = cJSON_GetObjectItem(root, "mqttPassword");
        if (item)
        {
            strncpy(mqttPassword, item->valuestring, strlen(item->valuestring));
        }
    }
    else
    {
        printf("parse json error\n");
    }
    cJSON_Delete(root);
    DEL(buffer);
    return 0;
}