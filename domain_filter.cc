#include "domain_filter.h"

#include <stdlib.h>
#include <assert.h>

#include "util.h"
#include "pdqsort.h"

using namespace std;

DomainFilter::DomainFilter()
{
    memset(list_, 0, 2 * 65536 * sizeof(char **) * DOMAIN_CHAR_COUNT);
    memset(list_count_, 0, 2 * 65536 * sizeof(int) * DOMAIN_CHAR_COUNT);
    p_ = NULL;
    size_ = 0;
    memset(list_range_, 0, 2 * 65536 * sizeof(int *) * DOMAIN_CHAR_COUNT);
}

DomainFilter::~DomainFilter()
{
    if (p_ != NULL)
    {
        p_ = p_ - BUFHEADSIZE;
        free(p_);
    }
    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 65536; ++j)
        {
            for (int k = 0; k < DOMAIN_CHAR_COUNT; ++k)
            {
                if (list_range_[i][j][k] != NULL)
                {
                    free(list_range_[i][j][k]);
                    list_range_[i][j][k] = NULL;
                }
                if (list_[i][j][k] != NULL)
                {
                    free(list_[i][j][k]);
                    list_[i][j][k] = NULL;
                }
            }
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
        int port = 0;
        char *offset = se - 3;
        int t = domain_temp[*offset];
        while (offset > s && offset > (se - 9))
        {
            if (*offset != ':')
                --offset;
            else
                break;
        }
        if (*offset == ':')
        {
            t = domain_temp[*(offset - 1)];
            ++offset;
            port = atoi(offset);
            // *(uint16_t *)(offset) = port;
        }
        ++this->list_count_[se[-1] == '-'][port][t];
        LOG("list_count_[%d][%d][%d]=%d\n", se[-1] == '-', port, t, list_count_[se[-1] == '-'][port][t]);
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
    int ***count = (int ***)malloc(2 * sizeof(int **));
    for (int i = 0; i < 2; i++)
    {
        count[i] = (int **)malloc(65536 * sizeof(int *));
        for (int j = 0; j < 65536; j++)
        {
            count[i][j] = (int *)malloc(DOMAIN_CHAR_COUNT * sizeof(int));
            memset(count[i][j], 0, DOMAIN_CHAR_COUNT);
        }
    }
    while (s < e)
    {
        char *se = strchr(s, '\n');
        int len = se - s - 2;
        char *offset = se - 3;
        int port = 0;
        int t = domain_temp[*offset];
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
            t = domain_temp[*(offset - 1)];
        }
        *(uint16_t *)(s - 2) = len - 1;
        bool hit = se[-1] == '-';
        this->list_[hit][port][t][count[hit][port][t]] = s;
        ++count[hit][port][t];
        s = se + 1;
    }
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 65536; j++)
        {
            free(count[i][j]);
        }
    }
    for (int i = 0; i < 2; i++)
    {
        free(count[i]);
    }
    free(count);
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
            for (int k = 0; k < DOMAIN_CHAR_COUNT; ++k)
            {
                if (filter->list_count_[i][j][k] > 0)
                    filter->list_[i][j][k] = (char **)malloc(filter->list_count_[i][j][k] * sizeof(char *));
            }
        }
    }

    filter->load_(p, size);
    filter->p_ = p;
    filter->size_ = size;
    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 65536; ++j)
        {
            for (int m = 0; m < DOMAIN_CHAR_COUNT; ++m)
            {
                if (filter->list_count_[i][j][m] > 0)
                {
                    pdqsort(filter->list_[i][j][m], filter->list_[i][j][m] + filter->list_count_[i][j][m], compare_dp_char);
                    prepare_range(filter->list_[i][j][m], filter->list_count_[i][j][m], filter->list_range_[i][j][m]);
#ifdef DEBUG
                    for (int k = 0; k < filter->list_count_[i][j][m]; ++k)
                    {
                        printf("domain:%s,port:%d,hit:%d\n", filter->list_[i][j][m][k], j, i);
                    }
#endif
                }
            }
        }
    }
    return filter;
}

int DomainFilter::merge(vector<DomainFilter *> domain_filter_list, int type)
{
    int count = 0;
    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 65536; ++j)
        {
            int size = 0;
            for (int k = 0; k < domain_filter_list.size(); ++k)
            {
                size += domain_filter_list[k]->list_count_[i][j][type];
            }
            if (size > 0)
            {
                count += size;
                list_[i][j][type] = (char **)malloc(size * sizeof(char *));
                list_count_[i][j][type] = size;
                size = 0;
                for (int k = 0; k < domain_filter_list.size(); ++k)
                {
                    if (domain_filter_list[k]->list_count_[i][j][type] != 0)
                    {
                        memcpy(list_[i][j][type] + size, domain_filter_list[k]->list_[i][j][type], domain_filter_list[k]->list_count_[i][j][type] * sizeof(char *));
                        size += domain_filter_list[k]->list_count_[i][j][type];
                    }
                }
                pdqsort(list_[i][j][type], list_[i][j][type] + list_count_[i][j][type], compare_dp_char);
                prepare_range(list_[i][j][type], list_count_[i][j][type], list_range_[i][j][type]);
            }
        }
    }
    return count;
}
