
#ifndef _CONFIG_h
#define _CONFIG_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

#define VERSION "2020.01.04.1100"

extern char httpPrefix[100];
extern char xiaoaiSN[20];
extern char zone[20];
extern char apiUrl[100];
extern char asrUrl[100];
extern char resUrl[100];
extern unsigned short asrLen;
extern unsigned short resLen;
extern unsigned short intervals;
extern int lastTime;
extern char asrKeywords[1024];
extern char resKeywords[1024];

extern char mqttServer[50];
extern unsigned short mqttPort;
extern char mqttUserName[30];
extern char mqttPassword[50];

int readConfig();

void updateItemToObject(cJSON *object, const char *string, cJSON *newitem);
#endif