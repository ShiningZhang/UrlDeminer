#include "prefix_filter.h"

#include <string.h>
#include <string>

#include "pdqsort.h"

#include "util.h"

using namespace std;

void PrefixFilter::load_(char *p, uint64_t size)
{

    char *e = p + size;
    char *s = p;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        int port_type = 0;
        if (s[0] == '/')
        {
            s += 2;
            port_type = 0; //80 | 443
        }
        else if (s[7] == '/')
        {
            s += 8;
            port_type = 2; // 443
        }
        else
        {
            s += 7;
            port_type = 1; // 80
        }
        char *domainend = strchr(s, '/');
        char *offset = domainend;
        while (offset > s && offset > (domainend - 6))
        {
            if (*offset != ':')
                --offset;
            else
                break;
        }

        if (*offset == ':')
        {
            char *offset_1 = offset + 1;
            int port_size = domainend - offset_1;
            if (port_type == 1 && port_size == 2 && memcmp(offset_1, "80", 2) == 0)
            {
                memmove(s + 3, s, offset - s - 1);
                s += 3;
            }
            else if (port_type == 2 && port_size == 3 && memcmp(offset_1, "443", 3) == 0)
            {
                memmove(s + 4, s, offset - s);
                s += 4;
            }
            else if (port_type == 0)
            {
                if (port_size == 2 && memcmp(offset_1, "80", 2) == 0)
                {
                    memmove(s + 3, s, offset - s);
                    s += 3;
                    port_type = 4;
                }
                else if (port_size == 3 && memcmp(offset_1, "443", 3) == 0)
                {
                    port_type = 5;
                }
            }
        }
        uint16_t len = se - s - 4;
        *(uint16_t *)(s - 2) = len + 3;
        uint8_t type = se[-3] == '=' ? 2 : se[-3] == '+' ? 1 : 0;
        bool hit = se[-1] == '-' ? true : false;
        if (port_type > 2)
        {
            s[len] = 1;
        }
        else
        {
            s[len] = 0xff & port_type;
        }
        if (port_type == 1)
        {
            list_http_[type][hit].push_back(s - 2);
            size_[type][hit][0] += len + 3;
        }
        else if (port_type == 2)
        {
            list_https_[type][hit].push_back(s - 2);
            size_[type][hit][1] += len + 3;
        }
        else if (port_type == 0)
        {
            list_http_[type][hit].push_back(s - 2);
            size_[type][hit][0] += len + 3;
            list_https_[type][hit].push_back(s - 2);
            size_[type][hit][1] += len + 3;
        }
        else if (port_type == 4)
        {
            list_http_[type][hit].push_back(s - 2);
            size_[type][hit][0] += len + 3;
            int str_length = 2 + (offset - s) + 3 + (se - domainend - 4) + 1;
            char *str = (char *)malloc(str_length);
            int tmp = 0;
            *((uint16_t *)(str)) = 0xffff & (len + 6);
            tmp += 2;
            memcpy(str + tmp, s, offset - s);
            tmp += offset - s;
            memcpy(str + tmp, ":80", 3);
            tmp += 3;
            memcpy(str + tmp, domainend, se - domainend - 4);
            tmp += se - domainend - 4;
            *(str + tmp) = 0xff & 2;
            list_str_.emplace_back(str);
            list_https_[type][hit].push_back(str);
            size_[type][hit][1] += len + 3;
        }
        else if (port_type == 5)
        {
            list_http_[type][hit].push_back(s - 2);
            size_[type][hit][0] += len + 3;
            int str_length = 2 + (offset - s) + (se - domainend - 4) + 1;
            char *str = (char *)malloc(str_length);
            int tmp = 0;
            *((uint16_t *)(str)) = 0xffff & (len + 3);
            tmp += 2;
            memcpy(str + tmp, s, offset - s);
            tmp += offset - s;
            memcpy(str + tmp, domainend, se - domainend - 4);
            tmp += se - domainend - 4;
            *(str + tmp) = 0xff & 2;
            list_str_.emplace_back(str);
            list_https_[type][hit].push_back(str);

            size_[type][hit][1] += len + 3;
        }
        // *se = '\0';
        // printf("PrefixFilter::load_:%d,%s\n", len, s);
        s = se + 1;
    }
}

PrefixFilter *PrefixFilter::load(char *p, uint64_t size)
{
    if (size == 0)
    {
        free(p);
        return NULL;
    }
    PrefixFilter *filter = new PrefixFilter();
    filter->load_(p, size);
    filter->p_ = p;
    filter->buf_size_ = size;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            pdqsort(filter->list_http_[i][j].begin(), filter->list_http_[i][j].end(), compare_prefix);
            pdqsort(filter->list_https_[i][j].begin(), filter->list_https_[i][j].end(), compare_prefix);
            /* for (int k = 0; k < filter->list_http_[i][j].size(); ++k)
            {
                printf("[%d,%d]http:%s\n", i, j, filter->list_http_[i][j][k] + 2);
            }
            for (int k = 0; k < filter->list_https_[i][j].size(); ++k)
            {
                printf("[%d,%d]https:%s\n", i, j, filter->list_https_[i][j][k] + 2);
            } */
        }
    }
    return filter;
}
