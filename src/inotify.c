#include "inotify.h"
#include <sys/inotify.h>
#include "http.h"
#include "config.h"
#include "util.h"
#include "ubus.h"
#include <regex.h>

void url_encode(struct mbuf *mb, const char *src)
{
    char c;
    const char *hex = "0123456789ABCDEF";
    size_t i = 0;
    for (i = 0; i < strlen(src); i++)
    {
        c = src[i];
        if (isalnum(c) || c == '.' || c == '-' || c == '_' || c == '*' || c == '(' || c == ')')
        {
            mbuf_append(mb, &c, 1);
        }
        else if (c == ' ')
        {
            mbuf_append(mb, "+", 1);
        }
        else
        {
            mbuf_append(mb, "%", 1);
            mbuf_append(mb, &hex[c >> 4], 1);
            mbuf_append(mb, &hex[c & 15], 1);
        }
    }
}

int doW()
{
    if (strlen(apiUrl) == 0)
    {
        return -1;
    }
    char *resBuffer = NULL, *asrBuffer = NULL;

    resBuffer = readFile("/tmp/mipns/mibrain/mibrain_txt_RESULT_NLP.log");
    if (resBuffer == NULL)
    {
        return -1;
    }

    char domain[20];
    getMid(resBuffer, (char *)"\"domain\":\"", '"', domain);

    int code = 0;
    char codeStr[20];
    if (getMid(resBuffer, (char *)"\"extend\":{\"code\":", ',', codeStr) > 0)
    {
        code = atoi(codeStr);
    }
    printf("== 有内容更新 | type: %s errcode: %d\n", domain, code);

    uint8_t api = code > 200 ? 1 : 0;

    if (code <= 200)
    {
        if (resKeywords != NULL && strlen(resKeywords) > 0 && matchRegex(resKeywords, resBuffer) == 1)
        {
            printf("== resKeywords\n");
            code = 2001;
        }
        if (code <= 200 && asrKeywords != NULL && strlen(asrKeywords) > 0)
        {
            asrBuffer = readFile("/tmp/mipns/mibrain/mibrain_asr.log");
            if (matchRegex(asrKeywords, asrBuffer) == 1)
            {
                printf("== asrKeywords\n");
                code = 2002;
            }
        }
    }

    if (code <= 200)
    {
        DEL(resBuffer);
        DEL(asrBuffer);
        return;
    }

    // @TODO: doamin是scenes的情况下,暂停播放,记录播放状态并暂停以及继续播放时有问题的
    // 为了保证不影响暂停效果,所以调整不同的停止播放方式
    printf("== 试图停止\n");
    uint8_t result;
    size_t i;
    // 若干循环,直到成功一次直接跳出
    for (i = 0; i < 200; i++)
    {
        result = playerPlayOperation(strcmp(domain, "scenes") == 0 ? "stop" : "resume");
        if (result == 0)
        {
            printf("== 停止成功\n");
            break;
        }
        usleep(50);
    }

    doStatus();
    // 记录播放状态并暂停,方便在HA服务器处理逻辑的时候不会插播音乐,0为未播放,1为播放中,2为暂停
    uint8_t playStatus = lastStatus;

    if (asrBuffer == NULL)
    {
        asrBuffer = readFile("/tmp/mipns/mibrain/mibrain_asr.log");
    }

    struct mbuf mb;
    mbuf_init(&mb, strlen(httpPrefix) + 9 + ((strlen(resBuffer) + strlen(asrBuffer))) * MBUF_SIZE_MULTIPLIER);
    //mbuf_append(&mb, "zone=", 5);
    //url_encode(&mb, zone);
    mbuf_append(&mb, httpPrefix, strlen(httpPrefix));
    mbuf_append(&mb, "&res=", 5);
    url_encode(&mb, resBuffer);
    mbuf_append(&mb, "&asr=", 5);
    url_encode(&mb, asrBuffer);

    DEL(resBuffer);
    DEL(asrBuffer);
    // 转发asr和res给服务端接口,远端可以处理控制逻辑完成后返回需要播报的TTS文本
    // 2秒连接超时,4秒传输超时

    struct httpState *state = (struct httpState *)malloc(sizeof(struct httpState));
    connectHhttp(state, apiUrl, CONTENTTYPE_POST, mb.buf);
    mbuf_free(&mb);
    printf("== 请求完成\n");
    if (state->httpCode == 200)
    {
        // 如果远端返回内容不为空则用TTS播报之
        if (strlen(state->data) > 0 && strlen(state->data) < 450)
        {
            if (playStatus == 1)
            {
                shellcmdNoResult("mphelper pause > /dev/null 2>&1");
                usleep(200 * 1000);
            }

            printf("== 播报TTS | TTS内容: %s\n", state->data);
            char resBuf[512];
            sprintf(resBuf, "ubus call mibrain text_to_speech \"{\\\"text\\\":\\\"%s\\\",\\\"save\\\":0}\" > /dev/null 2>&1", state->data);
            shellcmdNoResult(resBuf);
            usleep(200 * 1000);
            // 最长20秒TTS播报时间,20秒内如果播报完成跳出
            for (i = 0; i < 20; i++)
            {
                doStatus();
                if (lastPlayerType != 1)
                {
                    printf("== 播报TTS结束\n");
                    break;
                }
                usleep(1000 * 1000);
            }

            //如果之前音乐是播放的则接着播放
            if (playStatus == 1)
            {
                //这里延迟一秒是因为前面处理如果太快,可能引起恢复播放不成功
                usleep(1000 * 1000);
                printf("== 继续播放音乐\n");
                shellcmdNoResult("mphelper play > /dev/null 2>&1");
            }
        }
    }
    DEL(state->data);
    DEL(state);

    return 1;
}

int watch()
{
    const char *dirname = "/tmp/mipns/mibrain";
    const char *filename = "mibrain_txt_RESULT_NLP.log";

    char path[50];
    sprintf(path, "%s/%s", dirname, filename);

    int fd = inotify_init();
    if (fd == -1)
    {
        perror("inotify_init");
        return -1;
    }

    int dir_wd = inotify_add_watch(fd, dirname, IN_CREATE | IN_MOVED_TO | IN_MOVED_FROM);
    if (dir_wd == -1)
    {
        perror("inotify_add_watch");
        close(fd);
        return 0;
    }

    int file_wd = inotify_add_watch(fd, path, IN_CLOSE_WRITE | IN_DELETE_SELF);

    int len;
    char buf[512], *cur = buf, *end;
    while (1)
    {
        len = read(fd, cur, 512 - len);
        if (len <= 0)
        {
            perror("reading inotify event");
            break;
        }
        end = cur + len;
        while (cur + sizeof(struct inotify_event) <= end)
        {
            struct inotify_event *e = (struct inotify_event *)cur;
            if (cur + sizeof(struct inotify_event) + e->len > end)
            {
                break;
            }

            if (e->mask & IN_CREATE)
            {
                if (strcmp(e->name, filename) == 0 && !(e->mask & IN_ISDIR))
                {
                    printf("file %s is created.\n", filename);
                    file_wd = inotify_add_watch(fd, path, IN_CLOSE_WRITE | IN_DELETE_SELF);
                }
            }

            if (e->mask & IN_MOVED_FROM)
            {
                if (strcmp(e->name, filename) == 0 && !(e->mask & IN_ISDIR))
                {
                    printf("file %s is moved to other place.\n", filename);
                    inotify_rm_watch(fd, file_wd);
                }
            }

            if (e->mask & IN_MOVED_TO)
            {
                if (strcmp(e->name, filename) == 0 && !(e->mask & IN_ISDIR))
                {
                    printf("file %s is moved here from other place.\n", filename);
                    file_wd = inotify_add_watch(fd, path, IN_CLOSE_WRITE | IN_DELETE_SELF);
                }
            }

            if (e->mask & IN_CLOSE_WRITE)
            {
                doW();
                //printf("file %s is modified.\n", e->name);
            }

            if (e->mask & IN_DELETE_SELF && !(e->mask & IN_ISDIR))
            {
                printf("file %s is deleted.\n", filename);
                inotify_rm_watch(fd, file_wd);
            }
            cur += sizeof(struct inotify_event) + e->len;
        }

        if (cur >= end)
        {
            cur = buf;
            len = 0;
        }
        else
        {
            len = end - cur;
            memmove(buf, cur, len);
            cur = buf + len;
        }
    }
    close(fd);
    return 0;
}
