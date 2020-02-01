#include "http.h"

void http_ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
    struct http_message *hm = (struct http_message *)ev_data;
    struct httpState *state = (struct httpState *)nc->user_data;
    switch (ev)
    {
    case MG_EV_CONNECT:
        mg_set_timer(nc, 0); // Clear connect timer
        if (*(int *)ev_data != 0)
        {
            printf("Error connecting to server, error code: %d\n", (*(int *)ev_data));
            state->exitFlag = 1;
        }
        mg_set_timer(nc, mg_time() + 4);
        break;
    case MG_EV_HTTP_REPLY:
        mg_set_timer(nc, 0); // Clear connect timer
        state->httpCode = hm->resp_code;
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;

        state->data = (char *)malloc(hm->body.len + 1);
        memcpy(state->data, hm->body.p, hm->body.len);
        state->data[hm->body.len] = 0;
        state->exitFlag = 1;
        break;
    case MG_EV_TIMER:
        printf("Connect timeout\n");
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
        state->httpCode = 504;
        state->exitFlag = 1;
        break;
    case MG_EV_CLOSE:
        mg_set_timer(nc, 0); // Clear connect timer
        if (state->exitFlag == 0)
        {
            printf("Server closed connection\n");
            state->exitFlag = 1;
        }
        break;
    }
}

void connectHhttp(struct httpState *state, const char *url, const char *contentType, const char *params)
{
    struct mg_mgr mgr;
    state->exitFlag = 0;
    state->httpCode = 0;
    state->data = NULL;

    mg_mgr_init(&mgr, NULL);
    struct mg_connection *nc = mg_connect_http(&mgr, http_ev_handler, url, contentType, params); //POST模拟表单提交
    mg_set_timer(nc, mg_time() + 2);
    nc->user_data = state;
    while (state->exitFlag == 0)
    {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);
}