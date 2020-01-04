#include <stdio.h>
#include <pthread.h>
#include "cJSON.h"
#include "config.h"
#include "mongoose.h"
#include "ubus.h"
#include "util.h"
#include "http.h"
#include "inotify.h"
#include <sys/time.h>

char pubTopic[50];
struct mg_connection *mqttNc;
int mqttConnected = 0; // 0：没连上 1：已连上 2：已订阅
int mqttMessageId = 0;

void getSn()
{
    int len = shellcmd("matool_get_sn", xiaoaiSN, sizeof(xiaoaiSN));

    char tmp[75];
    urlencode(zone, strlen(zone), tmp, sizeof(tmp));
    sprintf(httpPrefix, "sn=%s&zone=%s", xiaoaiSN, tmp);

    printf("===============================================================\n");
    printf("               版本号: %s\n", VERSION);
    printf("           小爱序列号: %s\n", xiaoaiSN);
    printf("         语音接口地址: %s\n", apiUrl);
    printf("    asr拦截词接口地址: %s\n", asrUrl);
    printf("    res拦截词接口地址: %s\n", resUrl);
    printf("            asr拦截词: %s\n", strlen(asrKeywords) > 0 ? asrKeywords : "无拦截词");
    printf("            res拦截词: %s\n", strlen(resKeywords) > 0 ? resKeywords : "无拦截词");
    printf("       拦截词更新频率: ");
    if (intervals > 0)
    {
        printf("%d分钟\n", intervals);
    }
    else
    {
        printf("不更新\n");
    }
    printf("           MQTT服务器: %s:%d\n", mqttServer, mqttPort);
    printf("===============================================================\n");

    if (asrLen > 0)
    {
        strcat(asrUrl, strstr(asrUrl, "?") ? "&" : "?");
        strcat(asrUrl, httpPrefix);
    }
    if (resLen > 0)
    {
        strcat(resUrl, strstr(resUrl, "?") ? "&" : "?");
        strcat(resUrl, httpPrefix);
    }

    char *find_pos = strstr(xiaoaiSN, "/");
    if (find_pos)
    {
        *find_pos = '_';
    }
}

void load()
{
    readConfig();
    getSn();
}

bool subscribe()
{
    char topic[50];
    sprintf(topic, "xiaoai/%s/cmnd/#", xiaoaiSN);
    struct mg_mqtt_topic_expression s_topic_expr = {NULL, 0};
    s_topic_expr.topic = topic;
    printf("Subscribing to '%s'\n", topic);
    mg_mqtt_subscribe(mqttNc, &s_topic_expr, 1, mqttMessageId++);
    return true;
}

bool publish(char *topic, void *data, size_t len)
{
    if (mqttConnected != 2)
    {
        return false;
    }
    struct mg_mqtt_topic_expression s_topic_expr = {NULL, 0};
    s_topic_expr.topic = topic;
    mg_mqtt_publish(mqttNc, topic, mqttMessageId++, MG_MQTT_QOS(0), data, len);
    return true;
}

void doStatus()
{
    char strDst[3] = {0};
    uint8_t flag = getStatus();
    if ((1 & flag) == 1)
    {
        sprintf(pubTopic, "xiaoai/%s/stat/context", xiaoaiSN);
        publish(pubTopic, lastContext, strlen(lastContext));
    }
    if ((2 & flag) == 2)
    {
        sprintf(pubTopic, "xiaoai/%s/stat/volume", xiaoaiSN);
        sprintf(strDst, "%d", lastVolume);
        publish(pubTopic, strDst, strlen(strDst));
    }
    if ((8 & flag) == 8)
    {
        sprintf(pubTopic, "xiaoai/%s/stat/status", xiaoaiSN);
        sprintf(strDst, "%d", lastStatus);
        publish(pubTopic, strDst, strlen(strDst));
    }
}

void doSubscription(struct mg_mqtt_message *msg)
{
    char tmp[512];
    printf("Got incoming message %.*s: %.*s\n", (int)msg->topic.len, msg->topic.p, (int)msg->payload.len, msg->payload.p);
    if (endWith(msg->topic.p, "/tts", msg->topic.len))
    {
        sprintf(tmp, "ubus call mibrain text_to_speech \"{\\\"text\\\":\\\"%.*s\\\",\\\"save\\\":0}\" > /dev/null 2>&1", (int)msg->payload.len, msg->payload.p);
        shellcmdNoResult(tmp);
    }
    else if (endWith(msg->topic.p, "/cmd", msg->topic.len))
    {
        sprintf(tmp, "%.*s", (int)msg->payload.len, msg->payload.p);
        shellcmdNoResult(tmp);
    }
    else if (endWith(msg->topic.p, "/volume", msg->topic.len))
    {
        if ((msg->payload.len == 2 && strncmp(msg->payload.p, "up", msg->payload.len) == 0)       //up
            || (msg->payload.len == 4 && strncmp(msg->payload.p, "down", msg->payload.len) == 0)) //down
        {
            sprintf(tmp, "mphelper volume_%.*s > /dev/null 2>&1", (int)msg->payload.len, msg->payload.p);
            shellcmdNoResult(tmp);
        }
        else if (msg->payload.len > 0 && msg->payload.len < 4)
        {
            if (checkNum(msg->payload.p, msg->payload.len))
            {
                sprintf(tmp, "mphelper volume_set %.*s > /dev/null 2>&1", (int)msg->payload.len, msg->payload.p);
                shellcmdNoResult(tmp);
            }
        }
        doStatus();
    }
    else if (endWith(msg->topic.p, "/player", msg->topic.len))
    {
        if ((msg->payload.len == 2 && strncmp(msg->payload.p, "ch", msg->payload.len) == 0)         //ch
            || (msg->payload.len == 4 && strncmp(msg->payload.p, "prev", msg->payload.len) == 0)    //prev
            || (msg->payload.len == 4 && strncmp(msg->payload.p, "next", msg->payload.len) == 0)    //next
            || (msg->payload.len == 4 && strncmp(msg->payload.p, "play", msg->payload.len) == 0)    //play
            || (msg->payload.len == 5 && strncmp(msg->payload.p, "pause", msg->payload.len) == 0)   //pause
            || (msg->payload.len == 6 && strncmp(msg->payload.p, "toggle", msg->payload.len) == 0)) //toggle
        {
            sprintf(tmp, "mphelper %.*s > /dev/null 2>&1", (int)msg->payload.len, msg->payload.p);
            shellcmdNoResult(tmp);
        }
        else if ((msg->payload.len > 10 && strncmp(msg->payload.p, "http://", msg->payload.len) > 0)      // http://
                 || (msg->payload.len > 10 && strncmp(msg->payload.p, "https://", msg->payload.len) > 0)) // https://
        {
            sprintf(tmp, "mphelper tone %.*s > /dev/null 2>&1", (int)msg->payload.len, msg->payload.p);
            shellcmdNoResult(tmp);
        }
        doStatus();
    }
}

void ev_handler(struct mg_connection *nc, int ev, void *p)
{
    struct mg_mqtt_message *msg = (struct mg_mqtt_message *)p;
    (void)nc;
    switch (ev)
    {
    case MG_EV_CONNECT:
    {
        struct mg_send_mqtt_handshake_opts opts;
        memset(&opts, 0, sizeof(opts));
        opts.user_name = mqttUserName;
        opts.password = mqttPassword;

        mg_set_protocol_mqtt(nc);
        mg_send_mqtt_handshake_opt(nc, xiaoaiSN, opts);
        break;
    }
    case MG_EV_MQTT_CONNACK:
        if (msg->connack_ret_code != MG_EV_MQTT_CONNACK_ACCEPTED)
        {
            printf("Got mqtt connection error: %d\n", msg->connack_ret_code);
            mqttConnected = 4;
            nc->flags |= MG_F_CLOSE_IMMEDIATELY;
            break;
        }
        mqttConnected = 1;
        mqttMessageId = 1;
        mqttNc = nc;
        subscribe();
        break;
    case MG_EV_MQTT_PUBACK:
        printf("Message publishing acknowledged (msg_id: %d)\n", msg->message_id);
        break;
    case MG_EV_MQTT_SUBACK:
        mqttConnected = 2;
        printf("Subscription OK\n");
        break;
    case MG_EV_MQTT_PUBLISH:
        doSubscription(msg);
        break;
    case MG_EV_MQTT_DISCONNECT:
        mqttConnected = 0;
        printf("MG_EV_MQTT_DISCONNECT\n");
        break;
    case MG_EV_CLOSE:
        if (mqttConnected != 4)
        {
            mqttConnected = 0;
            //  printf("Connection closed\n");
        }
    }
}

void ev_http_handler(struct mg_connection *nc, int ev, void *p)
{
    if (ev == MG_EV_HTTP_REQUEST)
    {
        struct http_message *http = (struct http_message *)p;
        if (http->uri.len != 1)
        {
            mg_printf(nc, "HTTP/1.1 404 Not Found\r\nContent-Type: text/html;charset=utf-8\r\nServer: XiaoAi\r\nContent-Length: 9\r\n\r\nNot Found");
            nc->flags |= MG_F_SEND_AND_CLOSE;
            return;
        }

        char buf[512];
        memset(buf, 0, sizeof(buf));
        int len;
        if (mg_vcmp(&http->method, "POST") == 0)
        {
            char configFile[50];
            memset(configFile, 0, sizeof(configFile));
            getCurrentPath(configFile, "xiaoaimqtt.conf");
            char *buffer = readFile(configFile);
            if (buffer == NULL)
            {
                buffer = (char *)malloc(2);
                sprintf(buffer, "{}");
            }

            cJSON *root = cJSON_Parse(buffer);
            DEL(buffer);

            const char *list[] = {"zone", "apiUrl", "asrUrl", "resUrl", "mqttServer", "mqttUserName", "mqttPassword"};
            size_t i;
            for (i = 0; i < 7; i++)
            {
                memset(buf, 0, sizeof(buf));
                len = mg_get_http_var(&http->body, list[i], buf, sizeof(buf));
                buf[len] = 0;
                updateItemToObject(root, list[i], cJSON_CreateString(buf));
            }

            const char *list2[] = {"intervals", "mqttPort"};
            for (i = 0; i < 2; i++)
            {
                memset(buf, 0, sizeof(buf));
                len = mg_get_http_var(&http->body, list2[i], buf, sizeof(buf));
                buf[len] = 0;
                updateItemToObject(root, list2[i], cJSON_CreateNumber(atoi(buf)));
            }

            char *cjson_str = cJSON_Print(root);
            printf("%s\n", cjson_str);

            getCurrentPath(configFile, "xiaoaimqtt.conf");
            FILE *fp = fopen(configFile, "w");
            fprintf(fp, cjson_str);
            fclose(fp);

            free(cjson_str);
            cJSON_Delete(root);

            load();
            if (mqttConnected == 4)
            {
                mqttConnected = 0;
            }
            mg_printf(nc, "HTTP/1.1 302 Moved Temporarily\r\nServer: XiaoAi\r\nLocation: /?ok=1\r\n\r\n");
            nc->flags |= MG_F_SEND_AND_CLOSE;
            return;
        }
        len = mg_get_http_var(&http->query_string, "ok", buf, sizeof(buf));
        buf[len] = 0;

        char httpData[5 * 1024];
        sprintf(httpData, "<!DOCTYPE html><html lang='zh-cn'><head><meta charset='utf-8' /><meta name='viewport' content='width=device-width, initial-scale=1, user-scalable=no' /><title>小爱拦截、MQTT程序</title><style type='text/css'>bodybody{font-family:-apple-system,BlinkMacSystemFont,'Microsoft YaHei',sans-serif;font-size:16px;color:#333;line-height:1.75}table.gridtable{color:#333;border-width:1px;border-color:#ddd;border-collapse:collapse;margin:auto;margin-top:15px;width:80%}table.gridtable th{border-width:1.5px;padding:8px;border-style:solid;border-color:#ddd;background-color:#f5f5f5}table.gridtable td{border-width:1px;padding:8px;border-style:solid;border-color:#ddd;background-color:#fff}input{border:1px solid #ccc;padding:7px 0;border-radius:3px;padding-left:5px;-webkit-box-shadow:inset 0 1px 1px rgba(0,0,0,.075);box-shadow:inset 0 1px 1px rgba(0,0,0,.075);-webkit-transition:border-color ease-in-out .15s,-webkit-box-shadow ease-in-out .15s;-o-transition:border-color ease-in-out .15s,box-shadow ease-in-out .15s;transition:border-color ease-in-out .15s,box-shadow ease-in-out .15s}input:focus{border-color:#66afe9;outline:0;-webkit-box-shadow:inset 0 1px 1px rgba(0,0,0,.075),0 0 8px rgba(102,175,233,.6);box-shadow:inset 0 1px 1px rgba(0,0,0,.075),0 0 8px rgba(102,175,233,.6)}button{color:#fff;border-width:0;border-radius:3px;cursor:pointer;outline:0;font-size:17px;line-height:2.4rem;width:100%}.btn-info{background-color:#5bc0de;border-color:#46b8da}.btn-info:hover{background-color:#31b0d5;border-color:#269abc}.alert{width:80%;padding:15px;border:1px solid transparent;border-radius:4px;position:fixed;top:10px;left:10%;z-index:999999;display:none}</style><script type='text/javascript'>function toast(msg,duration,isok){var m=document.getElementById('alert');m.innerHTML=msg;m.style.cssText=isok?'color: #3c763d;background-color: #dff0d8;border-color: #d6e9c6;':'color: #a94442; background-color: #f2dede; border-color: #ebccd1;';m.style.display='block';setTimeout(function(){var d=0.5;m.style.webkitTransition='-webkit-transform '+d+'s ease-in, opacity '+d+'s ease-in';m.style.opacity='0';setTimeout(function(){m.style.display='none'},d*1000)},duration)};</script></head><body><div id='alert' class='alert'></div><h1 style='text-align: center'>小爱拦截、MQTT程序</h1><form method='post' action='/'><table class='gridtable'><thead><tr><th colspan='2'>小爱设置</th></tr></thead><tbody><tr><td>小爱位置</td><td><input type='text' name='zone' placeholder='小爱位置' value='%s'></td></tr><tr><td>接口地址</td><td><input type='text' name='apiUrl' placeholder='接口地址' value='%s' style='width:98%'></td></tr><tr><td>asr截词接口</td><td><input type='text' name='asrUrl' placeholder='asr截词接口' value='%.*s' style='width:98%'></td></tr><tr><td>res截词接口</td><td><input type='text' name='resUrl' placeholder='res截词接口' value='%.*s' style='width:98%'></td></tr><tr><td>更新时间</td><td><input type='number' min='0' max='1440' name='intervals' value='%d'>&nbsp;分钟</td></tr></tbody><thead><tr><th colspan='2'>MQTT设置</th></tr></thead><tbody><tr><td>地址</td><td><input type='text' name='mqttServer' placeholder='服务器地址' value='%s'></td></tr><tr><td>端口</td><td><input type='number' min='0' max='65535' name='mqttPort' value='%d'></td></tr><tr><td>用户名</td><td><input type='text' name='mqttUserName' placeholder='用户名' value='%s'></td></tr><tr><td>密码</td><td><input type='password' name='mqttPassword' placeholder='密码' value='%s'></td></tr><tr><td>状态</td><td>%s</td></tr><tr><td colspan='2'><button type='submit' class='btn-info'>保存</button></td></tr></tbody></table></form></body>%s</html>",
                zone,
                apiUrl,
                asrLen, asrUrl,
                resLen, resUrl,
                intervals,
                mqttServer,
                mqttPort,
                mqttUserName,
                mqttPassword,
                mqttConnected == 2 ? "已连接" : "未连接",
                len >= 1 ? "<script type='text/javascript'>toast('保存成功',5000,1)</script>" : "");

        mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: text/html;charset=utf-8\r\nServer: XiaoAi\r\nContent-Length: %zu\r\n\r\n%s", strlen(httpData), httpData);
        nc->flags |= MG_F_SEND_AND_CLOSE;
    }
}

void mqtt()
{
    char buf[60];
    struct mg_mgr mgr;
    mg_mgr_init(&mgr, NULL);

    struct mg_connection *httpNc = mg_bind(&mgr, "80", ev_http_handler);
    if (httpNc != NULL)
    {
        mg_set_protocol_http_websocket(httpNc);
    }

    for (;;)
    {
        if (mqttConnected == 0)
        {
            if (strlen(mqttServer) > 0 && mqttPort > 0)
            {
                mqttConnected = 3;
                sprintf(buf, "tcp://%s:%d", mqttServer, mqttPort);
                if (mg_connect(&mgr, buf, ev_handler) == NULL)
                {
                    fprintf(stderr, "mg_connect(%s) failed\n", buf);
                    usleep(30 * 1000 * 1000);
                }
            }
        }

        mg_mgr_poll(&mgr, 1000);
    }
}

void getKeyword()
{
    char configFile[50];
    memset(configFile, 0, sizeof(configFile));
    getCurrentPath(configFile, "xiaoaimqtt.conf");
    char *buffer = readFile(configFile);
    if (buffer == NULL)
    {
        buffer = (char *)malloc(2);
        sprintf(buffer, "{}");
    }

    cJSON *root = cJSON_Parse(buffer);
    DEL(buffer);

    struct httpState *state;

    bool isSave = false;
    size_t i;
    if (strlen(asrUrl) > 0)
    {
        state = (struct httpState *)malloc(sizeof(struct httpState));
        connectHhttp(state, asrUrl, NULL, NULL);
        if (state->httpCode == 200)
        {
            if (strlen(state->data) > 1020)
            {
                printf("max len is 1020  now %d\n", strlen(state->data));
            }
            else if (strlen(asrKeywords) == 0 && strlen(state->data) == 0)
            {
            }
            else if (strlen(asrKeywords) == 0 || strlen(asrKeywords) - 2 != strlen(state->data))
            {
                updateItemToObject(root, "asrKeywords", cJSON_CreateString(state->data));
                isSave = true;
            }
            else
            {
                for (i = 0; i < strlen(state->data); i++)
                {
                    if (state->data[i] != asrKeywords[i + 1])
                    {
                        updateItemToObject(root, "asrKeywords", cJSON_CreateString(state->data));
                        isSave = true;
                        break;
                    }
                }
            }
        }
        DEL(state->data);
        DEL(state);
    }

    if (strlen(resUrl) > 0)
    {
        state = (struct httpState *)malloc(sizeof(struct httpState));
        connectHhttp(state, resUrl, NULL, NULL);
        if (state->httpCode == 200)
        {
            if (strlen(state->data) > 1020)
            {
                printf("max len is 1020  now %d\n", strlen(state->data));
            }
            else if (strlen(resKeywords) == 0 && strlen(state->data) == 0)
            {
            }
            else if (strlen(resKeywords) == 0 || strlen(resKeywords) - 2 != strlen(state->data))
            {
                updateItemToObject(root, "resKeywords", cJSON_CreateString(state->data));
                isSave = true;
            }
            else
            {
                for (i = 0; i < strlen(state->data); i++)
                {
                    if (state->data[i] != resKeywords[i + 1])
                    {
                        updateItemToObject(root, "resKeywords", cJSON_CreateString(state->data));
                        isSave = true;
                        break;
                    }
                }
            }
        }
        DEL(state->data);
        DEL(state);
    }

    if (isSave)
    {
        char *cjson_str = cJSON_Print(root);
        FILE *fp = fopen(configFile, "w");
        fprintf(fp, cjson_str);
        fclose(fp);
        free(cjson_str);
        load();
    }

    cJSON_Delete(root);
}

void *run2(void *para)
{
    watch();
    return ((void *)0);
}

void *run1(void *para)
{
    struct timeval tv;
    while (1)
    {
        doStatus();
        gettimeofday(&tv, NULL);
        if (intervals > 0 && (lastTime + intervals * 60) < tv.tv_sec)
        {
            if (strlen(asrUrl) > 0 || strlen(asrUrl) > 0)
            {
                getKeyword();
            }

            lastTime = tv.tv_sec;
            //printf("next Time:%d\n", (lastTime + intervals * 60));
        }
        sleep(5);
    }
    return ((void *)0);
}

int main(void)
{
    load();

    pthread_t pid;
    pthread_create(&pid, NULL, run1, NULL);
    //pthread_join(pid, NULL);

    pthread_t pid2;
    pthread_create(&pid2, NULL, run2, NULL);

    mqtt();
    return 0;
}
