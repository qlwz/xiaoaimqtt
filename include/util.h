
#ifndef _UTIL_h
#define _UTIL_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NEW(p, size)         \
    {                        \
        p = calloc(1, size); \
    }

#define DEL(p)         \
    {                  \
        if (p != NULL) \
        {              \
            free(p);   \
            p = NULL;  \
        }              \
    }

int urldecode(char *str, int strSize, char *result, int resultSize);
int urlencode(char *str, int strSize, char *result, int resultSize);

int endWith(const char *str, const char *substr, int length_str);
int checkNum(const char *p, int len1);
int getMid(char *buf, char *left, char rigth, char *outBuf);
int matchRegex(const char *pattern, const char *userString);
char *readFile(char *path);
#endif