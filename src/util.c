
#include "util.h"
#include <regex.h>

int hex2num(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 10; //这里+10的原因是:比如16进制的a值为10
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 10;

    printf("unexpected char: %c", c);
    return '0';
}

/**
 * @brief URLDecode 对字符串URL解码,编码的逆过程
 *
 * @param str 原字符串
 * @param strSize 原字符串大小（不包括最后的\0）
 * @param result 结果字符串缓存区
 * @param resultSize 结果地址的缓冲区大小(包括最后的\0)
 *
 * @return: >0 result 里实际有效的字符串长度
 *            0 解码失败
 */
int urldecode(char *str, int strSize, char *result, int resultSize)
{
    if ((str == NULL) || (result == NULL) || (strSize <= 0) || (resultSize <= 0))
    {
        return 0;
    }
    char ch, ch1, ch2;
    int i;
    int j = 0; //record result index

    for (i = 0; (i < strSize) && (j < resultSize); ++i)
    {
        ch = str[i];
        switch (ch)
        {
        case '+':
            result[j++] = ' ';
            break;
        case '%':
            if (i + 2 < strSize)
            {
                ch1 = hex2num(str[i + 1]); //高4位
                ch2 = hex2num(str[i + 2]); //低4位
                if ((ch1 != '0') && (ch2 != '0'))
                    result[j++] = (char)((ch1 << 4) | ch2);
                i += 2;
                break;
            }
            else
            {
                break;
            }
        default:
            result[j++] = ch;
            break;
        }
    }

    result[j] = 0;
    return j;
}

/**
 * @brief URLEncode 对字符串URL编码
 *
 * @param str 原字符串
 * @param strSize 原字符串长度(不包括最后的\0)
 * @param result 结果缓冲区的地址
 * @param resultSize 结果缓冲区的大小(包括最后的\0)
 *
 * @return: >0:resultstring 里实际有效的长度
 *            0: 解码失败.
 */
int urlencode(char *str, int strSize, char *result, int resultSize)
{
    if ((str == NULL) || (result == NULL) || (strSize <= 0) || (resultSize <= 0))
    {
        return 0;
    }
    int i;
    int j = 0; //for result index
    char ch;

    for (i = 0; (i < strSize) && (j < resultSize); ++i)
    {
        ch = str[i];
        if (((ch >= 'A') && (ch < 'Z')) ||
            ((ch >= 'a') && (ch < 'z')) ||
            ((ch >= '0') && (ch < '9')))
        {
            result[j++] = ch;
        }
        else if (ch == ' ')
        {
            result[j++] = '+';
        }
        else if (ch == '.' || ch == '-' || ch == '_' || ch == '*')
        {
            result[j++] = ch;
        }
        else
        {
            if (j + 3 < resultSize)
            {
                sprintf(result + j, "%%%02X", (unsigned char)ch);
                j += 3;
            }
            else
            {
                return 0;
            }
        }
    }

    result[j] = '\0';
    return j;
}

int endWith(const char *str, const char *substr, int length_str)
{
    int res = 0;
    int length_sub = strlen(substr);
    int str_pos = length_str - length_sub;
    if (str_pos < 0)
    {
        return res;
    }
    int i = 0;
    for (i = 0; i < length_sub; i++)
    {
        if (str[i + str_pos] != substr[i])
        {
            break;
        }
    }
    if (i == length_sub)
    {
        res = 1;
    }
    return res;
}

int checkNum(const char *p, int len1)
{
    char v;
    int len = len1;
    while (len--)
    {
        v = *p++;
        if (v < '0' || v > '9')
        {
            return 0;
        }
    }
    len = len1;
    while (len--)
    {
        *p--;
    }
    return 1;
}

int getMid(char *buf, char *left, char rigth, char *outBuf)
{
    char v;
    int i = 0;
    char *pcBegin = strstr(buf, left);
    if (pcBegin)
    {
        pcBegin += strlen(left);
        i = 0;
        while (1)
        {
            v = *pcBegin++;
            if (v == rigth)
            {
                outBuf[i] = 0;
                break;
            }
            outBuf[i++] = v;
        }
    }
    return i;
}

int matchRegex(const char *pattern, const char *userString)
{
    int result = 0;
    regex_t regex;
    int regexInit = regcomp(&regex, pattern, REG_EXTENDED);
    if (regexInit)
    {
        //Error print : Compile regex failed
    }
    else
    {
        int reti = regexec(&regex, userString, 0, NULL, 0);
        if (REG_NOERROR != reti)
        {
            //Error print: match failed!
        }
        else
        {
            result = 1;
        }
    }
    regfree(&regex);
    return result;
}

char *readFile(char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
    {
        return NULL;
    }

    /* 获取文件大小 */
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    rewind(fp);

    /* 分配内存存储整个文件 */
    char *buffer = (char *)malloc(sizeof(char) * (length + 1));
    if (buffer == NULL)
    {
        return NULL;
    }

    /* 将文件拷贝到buffer中 */
    fread(buffer, 1, length, fp);
    buffer[length] = '\0';
    return buffer;
}