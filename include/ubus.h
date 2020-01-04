
#ifndef _UBUS_h
#define _UBUS_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern unsigned char lastVolume;     // 最后音量
extern unsigned char lastStatus;     // 最后状态
extern unsigned char lastPlayerType; // 最后播放类型
extern char lastContext[2046];       // 最后内容

unsigned char playerPlayOperation(char *oper);
unsigned char getStatus();
int shellcmd(char *cmd, char *buff, int size);
void shellcmdNoResult(char *cmd);
#endif