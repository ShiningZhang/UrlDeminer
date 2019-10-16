#include "domain_filter.h"

#include <stdlib.h>
#include <assert.h>

#include "util.h"
#include "pdqsort.h"

using namespace std;

DomainFilter::DomainFilter()
{
    memset(list_, 0, 2 * sizeof(char **) * DOMAIN_CHAR_COUNT);
    memset(list_count_, 0, 2 * sizeof(int) * DOMAIN_CHAR_COUNT);
    p_ = NULL;
    size_ = 0;
    memset(list_range_, 0, 2 * sizeof(int *) * DOMAIN_CHAR_COUNT);

    memset(list_port_, 0, 2 * sizeof(DomainPortBuf *));
    memset(list_port_count_, 0, 2 * sizeof(int));
}

DomainFilter::~DomainFilter()
{
    if (p_ != NULL)
    {
        p_ = p_ - BUFHEADSIZE;
        free(p_);
        p_ = NULL;
    }
    for (int i = 0; i < 2; ++i)
    {
        {
            for (int k = 0; k < DOMAIN_CHAR_COUNT; ++k)
            {
                if (list_range_[i][k] != NULL)
                {
                    free(list_range_[i][k]);
                    list_range_[i][k] = NULL;
                }
                if (list_[i][k] != NULL)
                {
                    free(list_[i][k]);
                    list_[i][k] = NULL;
                }
            }
        }
    }
    for (int i = 0; i < 2; ++i)
    {
        if (list_port_[i] != NULL)
        {
            free(list_port_[i]);
            list_port_[i] = NULL;
        }
    }
}

void DomainFilter::prepare_buf(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        char *offset = se - 3;
        int t = domain_temp[(int)((unsigned char)(*offset))];
        while (offset > s && offset > (se - 9))
        {
            if (*offset != ':')
                --offset;
            else
                break;
        }
        if (*offset == ':')
        {
            t = domain_temp[(int)((unsigned char)(*(offset - 1)))];
            ++this->list_port_count_[se[-1] == '-'];
        }
        else
        {
            ++this->list_count_[se[-1] == '-'][t];
        }
        // LOG("list_count_[%d][%d]=%d\n", se[-1] == '-',  t, list_count_[se[-1] == '-'][t]);
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
    int count[2][DOMAIN_CHAR_COUNT] = {0};
    int count_port[2] = {0};
    DomainPortBuf b;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        int len = se - s - 2;
        char *offset = se - 3;
        int port = 0;
        int t = domain_temp[(int)((unsigned char)(*offset))];
        bool hit = se[-1] == '-';
        while (offset > s && offset > (se - 9))
        {
            if (*offset != ':')
                --offset;
            else
                break;
        }
        if (*offset == ':')
        {
            port = atoi(offset + 1);
            // port = *(uint16_t *)(offset + 1);
            len = offset - s;
            t = domain_temp[(int)((unsigned char)(*(offset - 1)))];
            b.port = port;
            b.hit = hit;
            b.n = len;
            b.start = s;
            this->list_port_[hit][count_port[hit]++] = b;
            *(uint16_t *)(s - 2) = (uint16_t)(len);
#ifdef DEBUG
            printf("DomainFilter::load_:hit=%d,port=%d,n=%d,%s\n", b.hit, b.port, b.n, s);
#endif
        }
        else
        {
            *(uint16_t *)(s - 2) = (uint16_t)(len - 1);
            this->list_[hit][t][count[hit][t]] = s;
            ++count[hit][t];
#ifdef DEBUG
            printf("DomainFilter::load_:hit=%d,n=%d,t=%d,%s\n", hit, len - 1, t, s);
#endif
        }
        s = se + 1;
    }
}

void prepare_range(char **list, int size, int *&range)
{
    if (size == 0)
        return;
    range = (int *)malloc(size * sizeof(int));
    char *pa = list[0];
    range[0] = 1;
    for (int i = 1; i < size; ++i)
    {
        char *pb = list[i];
        if (compare_dp_char_eq(pa, pb) != 0)
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

DomainFilter *DomainFilter::load(char *p, uint64_t size)
{
    if (size == 0)
    {
        if (p != NULL)
        {
            p = p - BUFHEADSIZE;
            free(p);
        }
        return NULL;
    }
    DomainFilter *filter = new DomainFilter();
    filter->prepare_buf(p, size);
    for (int i = 0; i < 2; ++i)
    {
        {
            for (int k = 0; k < DOMAIN_CHAR_COUNT; ++k)
            {
                if (filter->list_count_[i][k] > 0)
                {
                    filter->list_[i][k] = (char **)malloc(filter->list_count_[i][k] * sizeof(char *));
#ifdef DEBUG
                    printf("DomainFilter::load:[%d,%d],count=%d\n", i, k, filter->list_count_[i][k]);
#endif
                }
            }
        }
    }
    for (int i = 0; i < 2; ++i)
    {
        if (filter->list_port_count_[i] > 0)
        {
            filter->list_port_[i] = (DomainPortBuf *)malloc(filter->list_port_count_[i] * sizeof(DomainPortBuf));
#ifdef DEBUG
            printf("DomainFilter::load:[%d],count=%d\n", i, filter->list_port_count_[i]);
#endif
        }
    }

    filter->load_(p, size);
    filter->p_ = p;
    filter->size_ = size;
    /*     for (int i = 0; i < 2; ++i)
    {
        {
            for (int m = 0; m < DOMAIN_CHAR_COUNT; ++m)
            {
                if (filter->list_count_[i][m] > 0)
                {
                    pdqsort(filter->list_[i][m], filter->list_[i][m] + filter->list_count_[i][m], compare_dp_char);
                    prepare_range(filter->list_[i][m], filter->list_count_[i][m], filter->list_range_[i][m]);
#ifdef DEBUG
                    for (int k = 0; k < filter->list_count_[i][m]; ++k)
                    {
                        printf("domain:%s,hit:%d\n", filter->list_[i][m][k], i);
                    }
#endif
                }
            }
        }
    } */
    return filter;
}

int DomainFilterMerge::merge(vector<DomainFilter *> domain_filter_list, int type)
{
    if (type >= DOMAIN_CHAR_COUNT)
    {
        return merge_port(domain_filter_list);
    }
    int count = 0;
    for (int i = 0; i < 2; ++i)
    {
        {
            int size = 0;
            for (size_t k = 0; k < domain_filter_list.size(); ++k)
            {
                size += domain_filter_list[k]->list_count_[i][type];
            }
            if (size > 0)
            {
                count += size;
                list_[i][type] = (char **)malloc(size * sizeof(char *));
                list_count_[i][type] = size;
                size = 0;
                for (size_t k = 0; k < domain_filter_list.size(); ++k)
                {
                    if (domain_filter_list[k]->list_count_[i][type] != 0)
                    {
                        memcpy(list_[i][type] + size, domain_filter_list[k]->list_[i][type], domain_filter_list[k]->list_count_[i][type] * sizeof(char *));
                        size += domain_filter_list[k]->list_count_[i][type];
                    }
                }
                pdqsort(list_[i][type], list_[i][type] + list_count_[i][type], compare_dp_char);
                prepare_range(list_[i][type], list_count_[i][type], list_range_[i][type]);
            }
        }
#ifdef DEBUG
        for (int tmp = 0; tmp < list_count_[i][type]; ++tmp)
        {
            printf("DomainFilterMerge::mergehit=%d,port=0,type=%d,%s\n", i, type, list_[i][type][tmp]);
        }
#endif
    }
    return count;
}

int DomainFilterMerge::merge_port(vector<DomainFilter *> domain_filter_list)
{
    int count = 0;
    for (int i = 0; i < 2; ++i)
    {
        int size = 0;
        for (size_t k = 0; k < domain_filter_list.size(); ++k)
        {
            size += domain_filter_list[k]->list_port_count_[i];
        }
        if (size > 0)
        {
            count += size;
            list_port_[i] = (DomainPortBuf *)malloc(size * sizeof(DomainPortBuf));
            list_port_count_[i] = size;
            size = 0;
            for (size_t k = 0; k < domain_filter_list.size(); ++k)
            {
                if (domain_filter_list[k]->list_port_count_[i] != 0)
                {
                    memcpy(list_port_[i] + size, domain_filter_list[k]->list_port_[i], domain_filter_list[k]->list_port_count_[i] * sizeof(DomainPortBuf));
                    size += domain_filter_list[k]->list_port_count_[i];
                }
            }
            pdqsort(list_port_[i], list_port_[i] + list_port_count_[i], compare_dp);
            {
                int port = 0;
                DomainPortBuf *list = list_port_[i];
                for (int m = 0; m < size; ++m)
                {
                    DomainPortBuf *p = list + m;
                    if (port != p->port)
                    {
                        port = p->port;
                        port_start_[i][port] = p;
                        ++port_size_[i][port];
                    }
                    else
                    {
                        ++port_size_[i][port];
                    }
                }
            }
            for (int port = 0; port < 65536; ++port)
            {
                if (port_size_[i][port] > 0)
                {
                    port_range_[i][port] = (int *)malloc(port_size_[i][port] * sizeof(int));
                    DomainPortBuf *pa = port_start_[i][port];
                    port_range_[i][port][0] = 1;
                    for (int m = 1; m < port_size_[i][port]; ++m)
                    {
                        DomainPortBuf *pb = port_start_[i][port] + m;
                        if (compare_dp_eq(pa, pb) != 0)
                        {
                            pa = pb;
                            port_range_[i][port][m] = 1;
                        }
                        else
                        {
                            port_range_[i][port][m] = port_range_[i][port][m - 1] + 1;
                        }
                    }
                }
            }
        }
#ifdef DEBUG
        for (int tmp = 0; tmp < list_port_count_[i]; ++tmp)
        {
            printf("DomainFilterMerge::merge_port:hit=%d,port=%d,n=%d,%s\n", list_port_[i][tmp].hit, list_port_[i][tmp].port, list_port_[i][tmp].n, list_port_[i][tmp].start);
        }
#endif
    }
    return count;
}

DomainFilterMerge::DomainFilterMerge()
{
    memset(port_start_, 0, 2 * 65536 * sizeof(DomainPortBuf *));
    memset(port_size_, 0, 2 * 65536 * sizeof(int));
    memset(port_range_, 0, 2 * 65536 * sizeof(int *));
}

DomainFilterMerge::~DomainFilterMerge()
{
    for (size_t i = 0; i < p_list_.size(); ++i)
    {
        if (p_list_[i] != NULL)
        {
            free(p_list_[i]);
        }
    }
    p_list_.clear();
    buf_size_list_.clear();
    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 65536; ++j)
        {
            if (port_range_[i][j] != NULL)
            {
                free(port_range_[i][j]);
            }
        }
    }
}

void DomainFilterMerge::cpy_filter_list(vector<DomainFilter *> &list)
{
    for (uint i = 0; i < list.size(); ++i)
    {
        p_list_.push_back(list[i]->p_ - BUFHEADSIZE);
        buf_size_list_.push_back(list[i]->size_);
        list[i]->p_ = NULL;
        list[i]->size_ = 0;
        delete list[i];
    }
    list.clear();
}