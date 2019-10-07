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
        b.hit = s[len - 1] == '-';
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

DomainFilter *DomainFilter::load(char *p, uint64_t size)
{
    DomainFilter *filter = new DomainFilter();
    filter->load_(p, size);
    filter->p_ = p;
    filter->size_ = size;
    pdqsort(filter->list_.begin(), filter->list_.end(), compare_dp);
    return filter;
}