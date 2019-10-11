#include "domain_filter.h"

#include <stdlib.h>
#include <assert.h>

#include "util.h"
#include "pdqsort.h"

using namespace std;

DomainFilter::DomainFilter()
{
    memset(list_count_, 0, 2 * 65536 * sizeof(int));
    p_ = NULL;
    size_ = 0;
}

DomainFilter::~DomainFilter()
{
    if (p_ != NULL)
    {
        p_ = p_ - BUFHEADSIZE;
        free(p_);
    }
}

void DomainFilter::prepare_buf(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        int port = 0;
        char *offset = se - 3;
        while (offset > s && offset > (se - 9))
        {
            if (*offset != ':')
                --offset;
            else
                break;
        }
        if (*offset == ':')
        {
            ++offset;
            port = atoi(offset);
            *(uint16_t *)(offset) = port;
        }
        ++this->list_count_[se[-1] == '-'][port];
        s = se + 1;
    }
}

/* void DomainFilter::load_(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    DomainPortBuf b;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        int len = se - s;
        b.hit = (int)(s[len - 1] == '-');
        s[len - 2] = '\0';
        char *port = strchr(s, ':');
        if (port != NULL)
        {
            b.port = atoi(port + 1);
            len = port - s;
        }
        else
        {
            b.port = 0;
            len = len - 2;
        }
        b.n = len;
        b.start = s;
        list_.push_back(b);
#ifdef DEBUG
        printf("load_:domain:%10s,n:%d,port:%d,hit:%d\n", b.start, b.n, b.port, b.hit);
#endif
        s = se + 1;
    }
} */

void DomainFilter::load_(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        int len = se - s - 2;
        char *offset = se - 3;
        int port = 0;
        while (offset > s && offset > (se - 9))
        {
            if (*offset != ':')
                --offset;
            else
                break;
        }
        if (*offset == ':')
        {
            port = *(uint16_t *)(offset + 1);
            len = offset - s;
        }
        *(uint16_t *)(s - 2) = len;
        bool hit = se[-1] == '-';
        this->list_[hit][port][this->list_count_[hit][port]] = s;
        ++this->list_count_[hit][port];
        s = se + 1;
    }
}

DomainFilter *DomainFilter::load(char *p, uint64_t size)
{
    if (size == 0)
    {
        p = p - BUFHEADSIZE;
        free(p);
        return NULL;
    }
    DomainFilter *filter = new DomainFilter();
    filter->prepare_buf(p, size);
    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 65536; ++j)
        {
            filter->list_[i][j] = (char **)malloc(filter->list_count_[i][j] * sizeof(char *));
        }
    }
    memset(filter->list_count_, 0, 2 * 65536 * sizeof(int));

    filter->load_(p, size);
    filter->p_ = p;
    filter->size_ = size;
    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 65536; ++j)
        {
            if (filter->list_count_[i][j] > 0)
            {
                pdqsort(filter->list_[i][j], filter->list_[i][j] + filter->list_count_[i][j], compare_dp_char);
#ifdef DEBUG
                for (int k = 0; k < filter->list_count_[i][j]; ++k)
                {
                    printf("domain:%s,port:%d,hit:%d\n", filter->list_[i][j][k], j, i);
                }
#endif
            }
        }
    }
    /*     pdqsort(filter->list_.begin(), filter->list_.end(), compare_dp);
#ifdef DEBUG
    for (int i = 0; i < filter->list_.size(); ++i)
    {
        printf("domain:%10s,n:%d,port:%d,hit:%d\n", filter->list_[i].start, filter->list_[i].n, filter->list_[i].port, filter->list_[i].hit);
    }
#endif */
    return filter;
}