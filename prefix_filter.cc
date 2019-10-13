#include "prefix_filter.h"

#include <string.h>
#include <string>

#include "pdqsort.h"

#include "util.h"

using namespace std;

PrefixFilter::PrefixFilter()
{
    p_ = NULL;
    buf_size_ = 0;
    memset(size_, 0, 3 * 2 * 2 * sizeof(uint64_t));
    memset(list_range_, 0, 3 * 2 * 2 * sizeof(int *));
    memset(list_count_, 0, 3 * 2 * 2 * sizeof(int));
}

PrefixFilter::~PrefixFilter()
{
    if (p_ != NULL)
    {
        p_ = p_ - BUFHEADSIZE;
        free(p_);
        p_ = NULL;
    }
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < 2; ++k)
            {
                if (list_count_[i][j][k] > 0)
                {
                    free(list_https_[i][j][k]);
                    list_https_[i][j][k] = NULL;
                }
                if (list_range_[i][j][k] != NULL)
                {
                    free(list_range_[i][j][k]);
                    list_range_[i][j][k] = NULL;
                }
            }
        }
    }
}

void PrefixFilter::prepare_buf(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        uint8_t type = se[-3] == '=' ? 2 : se[-3] == '+' ? 1 : 0;
        bool hit = se[-1] == '-' ? true : false;
        if (s[0] == '/')
        {
            ++this->list_count_[type][hit][0];
            ++this->list_count_[type][hit][1];
        }
        else if (s[7] == '/')
        {
            ++this->list_count_[type][hit][1];
        }
        else
        {
            ++this->list_count_[type][hit][0];
        }
        s = se + 1;
    }
}

/* void PrefixFilter::load_(char *p, uint64_t size)
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
        *(uint16_t *)(s - 2) = len;
        uint8_t type = se[-3] == '=' ? 2 : se[-3] == '+' ? 1 : 0;
        bool hit = se[-1] == '-' ? true : false;
        if (port_type > 2)
        {
            s[-3] = 1;
        }
        else
        {
            s[-3] = 0xff & port_type;
        }
        if (port_type == 1)
        {
            list_https_[type][hit][0].push_back(s - 2);
            size_[type][hit][0] += len + 3;
        }
        else if (port_type == 2)
        {
            list_https_[type][hit][1].push_back(s - 2);
            size_[type][hit][1] += len + 3;
        }
        else if (port_type == 0)
        {
            list_https_[type][hit][0].push_back(s - 2);
            size_[type][hit][0] += len + 3;
            list_https_[type][hit][1].push_back(s - 2);
            size_[type][hit][1] += len + 3;
        }
        else if (port_type == 4)
        {
            list_https_[type][hit][0].push_back(s - 2);
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
            ++tmp;
            arrangesuffix(str + 2, len + 3);
            list_str_.emplace_back(str);
            list_https_[type][hit][1].push_back(str);
            size_[type][hit][1] += len + 6;
        }
        else if (port_type == 5)
        {
            list_https_[type][hit][0].push_back(s - 2);
            size_[type][hit][0] += len + 3;
            int str_length = 2 + (offset - s) + (se - domainend - 4) + 1;
            char *str = (char *)malloc(str_length);
            int tmp = 0;
            *((uint16_t *)(str)) = 0xffff & (len - 1);
            tmp += 2;
            memcpy(str + tmp, s, offset - s);
            tmp += offset - s;
            memcpy(str + tmp, domainend, se - domainend - 4);
            tmp += se - domainend - 4;
            *(str + tmp) = 0xff & 2;
            arrangesuffix(str + 2, len - 4);
            list_str_.emplace_back(str);
            list_https_[type][hit][1].push_back(str);
            size_[type][hit][1] += len - 1;
        }
        arrangesuffix(s, len);
#ifdef DEBUG
        *se = '\0';
        printf("PrefixFilter::load_:%d,%s\n", len, s);
#endif
        s = se + 1;
    }
} */

void PrefixFilter::load_(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    int count[3][2][2];
    memset(count, 0, 3 * 2 * 2 * sizeof(int));
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
        *(uint16_t *)(s - 2) = len;
        uint8_t type = se[-3] == '=' ? 2 : se[-3] == '+' ? 1 : 0;
        bool hit = se[-1] == '-' ? true : false;
        if (port_type > 2)
        {
            s[-3] = 1;
        }
        else
        {
            s[-3] = 0xff & port_type;
        }
        if (port_type == 1)
        {
            list_https_[type][hit][0][count[type][hit][0]] = (s - 2);
            size_[type][hit][0] += len + 3;
            ++count[type][hit][0];
        }
        else if (port_type == 2)
        {
            list_https_[type][hit][1][count[type][hit][1]] = (s - 2);
            size_[type][hit][1] += len + 3;
            ++count[type][hit][1];
        }
        else if (port_type == 0)
        {
            list_https_[type][hit][0][count[type][hit][0]] = (s - 2);
            size_[type][hit][0] += len + 3;
            ++count[type][hit][0];
            list_https_[type][hit][1][count[type][hit][1]] = (s - 2);
            size_[type][hit][1] += len + 3;
            ++count[type][hit][1];
        }
        else if (port_type == 4)
        {
            list_https_[type][hit][0][count[type][hit][0]] = (s - 2);
            size_[type][hit][0] += len + 3;
            ++count[type][hit][0];
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
            ++tmp;
            arrangesuffix(str + 2, len + 3);
            list_str_.emplace_back(str);
            list_https_[type][hit][1][count[type][hit][1]] = str;
            size_[type][hit][1] += len + 6;
            ++count[type][hit][1];
        }
        else if (port_type == 5)
        {
            list_https_[type][hit][0][count[type][hit][0]] = (s - 2);
            ++count[type][hit][0];
            size_[type][hit][0] += len + 3;
            int str_length = 2 + (offset - s) + (se - domainend - 4) + 1;
            char *str = (char *)malloc(str_length);
            int tmp = 0;
            *((uint16_t *)(str)) = 0xffff & (len - 1);
            tmp += 2;
            memcpy(str + tmp, s, offset - s);
            tmp += offset - s;
            memcpy(str + tmp, domainend, se - domainend - 4);
            tmp += se - domainend - 4;
            *(str + tmp) = 0xff & 2;
            arrangesuffix(str + 2, len - 4);
            list_str_.emplace_back(str);
            list_https_[type][hit][1][count[type][hit][1]] = str;
            ++count[type][hit][1];
            size_[type][hit][1] += len - 1;
        }
        arrangesuffix(s, len);
#ifdef DEBUG
        *se = '\0';
        printf("PrefixFilter::load_:%d,%s\n", len, s);
#endif
        s = se + 1;
    }
}

PrefixFilter *PrefixFilter::load(char *p, uint64_t size)
{
    if (size == 0)
    {
        p = p - BUFHEADSIZE;
        free(p);
        return NULL;
    }
    PrefixFilter *filter = new PrefixFilter();
    filter->prepare_buf(p, size);
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < 2; ++k)
            {
                if (filter->list_count_[i][j][k] > 0)
                {
                    filter->list_https_[i][j][k] = (char **)malloc(filter->list_count_[i][j][k] * sizeof(char *));
                }
            }
        }
    }
    filter->load_(p, size);
    filter->p_ = p;
    filter->buf_size_ = size;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            pdqsort(filter->list_https_[i][j][0], filter->list_https_[i][j][0] + filter->list_count_[i][j][0], compare_prefix);
            pdqsort(filter->list_https_[i][j][1], filter->list_https_[i][j][1] + filter->list_count_[i][j][1], compare_prefix);
        }
    }
    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < 2; ++k)
            {
                filter->prepare_range(filter->list_https_[i][j][k], filter->list_count_[i][j][k], filter->list_range_[i][j][k]);
            }
        }
    }
    return filter;
}

void PrefixFilter::prepare_range(char **list, int size, int *&range)
{
    if (size == 0)
        return;
    range = (int *)malloc(size * sizeof(int));
    char *pa = list[0];
    range[0] = 1;
    for (int i = 1; i < size; ++i)
    {
        char *pb = list[i];
        if (compare_prefix_eq(pa, pb) != 0)
        {
            pa = pb;
            range[i] = 1;
        }
        else
        {
            range[i] = range[i - 1] + 1;
        }
    }
}

PrefixFilter *PrefixFilter::merge(vector<PrefixFilter *> prefix_filter_list)
{
    PrefixFilter *filter = new PrefixFilter();
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < 2; ++k)
            {
                int size = 0;
                for (int m = 0; m < prefix_filter_list.size(); ++m)
                {
                    size += prefix_filter_list[m]->list_count_[i][j][k];
                }
                if (size > 0)
                {
                    filter->list_count_[i][j][k] = size;
                    filter->list_https_[i][j][k] = (char **)malloc(filter->list_count_[i][j][k] * sizeof(char *));
                    size = 0;
                    for (int m = 0; m < prefix_filter_list.size(); ++m)
                    {
                        memcpy(filter->list_https_[i][j][k] + size, prefix_filter_list[m]->list_https_[i][j][k], prefix_filter_list[m]->list_count_[i][j][k] * sizeof(char *));
                        size += prefix_filter_list[m]->list_count_[i][j][k];
                    }
                    pdqsort(filter->list_https_[i][j][k], filter->list_https_[i][j][k] + filter->list_count_[i][j][k], compare_prefix);
                    filter->prepare_range(filter->list_https_[i][j][k], filter->list_count_[i][j][k], filter->list_range_[i][j][k]);
#ifdef DEBUG
                    for (int n = 0; n < filter->list_count_[i][j][k]; ++n)
                    {
                        printf("http[%d,%d,%d]:%s\n", i, j, k, *(filter->list_https_[i][j][k] + n) + 2);
                    }
#endif
                }
            }
        }
    }
    return filter;
}