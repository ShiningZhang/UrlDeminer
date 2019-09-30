#include "domain_filter.h"

#include <stdlib.h>
#include <assert.h>

#include "util.h"
#include "pdqsort.h"

using namespace std;

void DomainFilter::load_(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    DomainPortBuf b;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        int len = se - s;
        b.allow = s[len - 1] == '+';
        s[len - 2] = '\0';
        char *port = strchr(s, ':');
        if (port != NULL)
        {
            sscanf(port + 1, "%d", &b.port);
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

        s = se + 1;
    }
}

static inline int cmp64val(int64_t ia, int64_t ib)
{
    int64_t sub = ia - ib;
    if (sub < 0)
    {
        return -1;
    }
    else if (sub > 0)
    {
        return 1;
    }
    return 0;
}

static int cmpbuf(const char *pa, int na, const char *pb, int nb)
{
    while (na >= 8 && nb >= 8)
    {
        na -= 8;
        nb -= 8;
        int64_t ia = *(int64_t *)(pa + na);
        int64_t ib = *(int64_t *)(pb + nb);
        int ret = cmp64val(ia, ib);
        if (ret != 0)
        {
            return ret;
        }
    }
    while (na >= 0 && nb >= 0)
    {
        int sub = (int)((unsigned char)(*(pa + na))) - (int)((unsigned char)(*(pb + nb)));
        if (sub < 0)
        {
            return -1;
        }
        else if (sub > 0)
        {
            return 1;
        }
        --na;
        --nb;
    }
    return 0;
}

bool compare_char(const DomainPortBuf &e1, const DomainPortBuf &e2)
{
    const char *pa = e1.start;
    const char *pb = e2.start;
    uint16_t na = e1.n;
    uint16_t nb = e2.n;
    int ret = cmpbuf(pa, na, pb, nb);
    if (ret != 0)
        return ret == -1 ? true : false;
    if (e1.port < e2.port)
        return true;
    else if (e1.port > e2.port)
        return false;
    if (na < nb)
        return true;
    else if (na > nb)
        return false;
    if (e1.allow > e2.allow)
        return true;
    else if (e1.allow < e2.allow)
        return false;
    return pa < pb;
}

DomainFilter *DomainFilter::load(char *p, uint64_t size)
{
    DomainFilter *filter = new DomainFilter();
    filter->load_(p, size);
    p_ = p;
    size_ = size;
    pdqsort(filter->list_.begin(), filter->list_.end(), compare_char);
    return filter;
}