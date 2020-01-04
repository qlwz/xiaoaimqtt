
#ifndef _HTTP_h
#define _HTTP_h

#include <stdio.h>
#include "mongoose.h"

#define CONTENTTYPE_POST "Content-Type: application/x-www-form-urlencoded\r\n"
#define CONTENTTYPE_JSON "Content-Type: application/json;charset=utf-8\r\n"

struct httpState
{
    int exitFlag;
    int httpCode;
    char *data;
};

void connectHhttp(struct httpState *state, const char *url, const char *contentType, const char *params);
#endif