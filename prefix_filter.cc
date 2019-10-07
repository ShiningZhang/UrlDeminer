#include "prefix_filter.h"

#include <string.h>
#include <string>

using namespace std;

void PrefixFilter::load_(char *p, uint64_t size)
{
    
    char *e = p + size;
    char *s = p;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        int port_type = 0;
        if(memcmp(s, "//", 2) == 0)
        {
            s += 2;
            port_type = 0; //80 | 443
        } else if(memcmp(s, "https", 5) == 0)
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
        char *offset = s;
        while(offset < domainend && *offset != ':')
            ++offset;
        if (offset < domainend)
        {
            ++offset;
            int port_size = domainend - offset;
            if (port_type == 1 && port_size == 2 && memcmp(offset, "80", 2) == 0)
            {
                memmove(s+3, s, offset-s-1);
                s += 3;
            } else if(port_type == 2 && port_size == 3 && memcmp(offset, "443", 3) == 0)
            {
                memmove(s+4, s, offset-s-1);
                s += 4;
            } else if(port_type == 0 )
            {
                if (port_size == 2 && memcmp(offset, "80", 2) == 0)
                {
                    memmove(s+3, s, offset-s-1);
                    s += 3;
                    port_type = 4;
                } else if(port_size == 3 && memcmp(offset, "443", 3) == 0)
                {
                    port_type = 5;
                }
            }
        }
        uint16_t len = se - s - 4;
        *(uint16_t *)(s-2) = len + 3;
        uint8_t type = se[-3] == '=' ? 2 : se[-3] == '+' ? 1:0;
        bool hit = se[-1] == '-' ? true: false;
        if (port_type > 2)
        {
            s[len] = 1;
        } else
        {
            s[len] = 0xff & port_type;
        }
        if (port_type == 1)
        {
            list_http_[type][hit].push_back(s-2);
            size_[type][hit][0] += len + 3;
        } else if(port_type == 2)
        {
            list_https_[type][hit].push_back(s-2);
            size_[type][hit][1] += len + 3;
        } else if(port_type == 0)
        {
            list_http_[type][hit].push_back(s-2);
            size_[type][hit][0] += len + 3;
            list_https_[type][hit].push_back(s-2);
            size_[type][hit][1] += len + 3;
        }
        else if (port_type == 4)
        {
            list_http_[type][hit].push_back(s-2);
            size_[type][hit][0] += len + 3;
            string str;
            str.append((char*)&(uint16_t)(0xffff & (len+6)), 2);
            str.append(s, offset-s-1);
            str.append(":80", 3);
            str.append(domainend, se-domainend-4);
            str.append((char*)&(char)(0xff&2), 1);
            list_str_.emplace_back(str);
            list_https_[type][hit].push_back(str.data());
            size_[type][hit][1] += len + 3;
        } else if (port_type == 5)
        {
            list_http_[type][hit].push_back(s-2);
            size_[type][hit][0] += len + 3;
            string str();
            str.append((char*)&(uint16_t)(0xffff & (len)), 2);
            str.append(s, offset-s-1);
            str.append(domainend, se-domainend-4);
            str.append((char*)&(char)(0xff&2), 1);
            list_str_.emplace_back(str);
            list_https_[type][hit].push_back(str.data());
            size_[type][hit][1] += len + 3;
        }
        s = se + 1;
    }
}


PrefixFilter *PrefixFilter::load(char *p, uint64_t size)
{
    PrefixFilter *filter = new PrefixFilter();
    filter->load_(p, size);
    filter->p_ = p;
    filter->size_ = size;
    for (int i=0;i<3;++i)
    {
        for(int j=0;j<2;++j)
        {
            pdqsort(filter->list_http_[i][j].begin(), filter->list_http_[i][j].end(), compare_prefix);
            pdqsort(filter->list_https_[i][j].begin(), filter->list_https_[i][j].end(), compare_prefix);
        }
    }
    return filter;
}
