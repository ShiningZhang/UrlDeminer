#include "url_filter.h"

#include <string.h>
#include <string>

using namespace std;

void UrlFilter::load_(char *p, uint64_t size)
{
    
    char *e = p + size;
    char *s = p;
    DomainPortBuf b;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        if (s[7] == '/')
        {
        }
        else
        {
        }
        int len = se -s;
        
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
    int ret = 0;
    while (na >= 8 && nb >= 8)
    {
        int64_t ia = *(int64_t *)pa;
        int64_t ib = *(int64_t *)pb;
        ret = cmp64val(ia, ib);
        if (ret != 0)
        {
            return ret;
        }
        na -= 8;
        nb -= 8;
        pa += 8;
        pb += 8;
    }
    int nc = min(na, nb);
    return memcmp(pa, pb, nc);
}

bool compare_prefix(const char* e1, const char *e2)
{
    const char *pa = e1;
    const char *pb = e2;
    int na = (int)*((uint16_t*)pa) - 3;
    int nb = (int)*((uint16_t*)pb) - 3;
    pa +=2;
    pb+=2;
    int ret = cmpbuf(pa, na, pb, nb);
    if (ret != 0)
        return ret == -1 ? true : false;
    if (na < nb)
        return true;
    else if (na > nb)
        return false;
    return e1<e2;
}


UrlFilter *UrlFilter::load(char *p, uint64_t size)
{
    UrlFilter *filter = new UrlFilter();
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
