#include "url_filter.h"

#include <string.h>
#include <string>

#include <algorithm>
#include <utility>

#include "domain_filter.h"
#include "prefix_filter.h"

#include <stdio.h>

#include "Global_Macros.h"

using namespace std;

UrlFilter::UrlFilter() : p_(NULL), size_(0), buf_size_(0), out_(NULL), out_size_(0), out_offset_(0)
{
    memset(list_, 0, DOMAIN_CHAR_COUNT * sizeof(char **));
    memset(list_count_, 0, DOMAIN_CHAR_COUNT * sizeof(int));
    memset(max_list_count_, 0, DOMAIN_CHAR_COUNT * sizeof(int));
    memset(list_domainport_, 0, DOMAIN_CHAR_COUNT * sizeof(DomainPortBuf *));
    memset(list_domainport_count_, 0, DOMAIN_CHAR_COUNT * sizeof(int));
    memset(max_list_domainport_count_, 0, DOMAIN_CHAR_COUNT * sizeof(int));
}

UrlFilter::~UrlFilter()
{
    for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
    {
        if (list_[i] != NULL)
        {
            free(list_[i]);
            list_[i] = NULL;
        }
        if (list_domainport_[i] != NULL)
        {
            free(list_domainport_[i]);
            list_domainport_[i] = NULL;
        }
    }
    if (p_ != NULL)
    {
        p_ = p_ - BUFHEADSIZE;
        free(p_);
        p_ = NULL;
    }
}

int UrlFilter::prepare_buf(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    int count = 0;
    int domain_count[DOMAIN_CHAR_COUNT] = {0};
    int prefix_count[DOMAIN_CHAR_COUNT] = {0};
    while (s < e)
    {
        char *se = strchr(s, '\n');
        s += 8;
        char *domain_end = strchr(s, '/');
        char *port = domain_end - 1;
        domain_end = s > (domain_end - 6) ? s : (domain_end - 6);
        int t = domain_temp[(unsigned char)(*port)];
        while (port > domain_end)
        {
            if (*port != ':')
                --port;
            else
                break;
        }
        if (*port == ':')
            t = domain_temp[(unsigned char)*(port - 1)];
        ++domain_count[t];
        ++count;
        if (s[-1] == '/')
            t = domain_temp[(unsigned char)*s];
        else
            t = domain_temp[(unsigned char)s[-1]];
        ++prefix_count[t];
        s = se + 1;
    }
    for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
    {
        this->list_domainport_count_[i] = domain_count[i];
        this->list_count_[i] = prefix_count[i];
    }
    return count;
}

int UrlFilter::load_(char *p, uint64_t size)
{

    char *e = p + size;
    char *s = p;
    DomainPortBuf b;
    int count[DOMAIN_CHAR_COUNT] = {0};
    // memset(count, 0, DOMAIN_CHAR_COUNT * sizeof(int));
    while (s < e)
    {
        char *se = strchr(s, '\n');
        if (s[7] == '/')
        {
            s += 8;
            s[-3] = 1 & 0xff; //https
            b.port = 443;
        }
        else
        {
            s += 7;
            s[-3] = 0 & 0xff; //http
            b.port = 80;
        }
        char *domain_end = strchr(s, '/');
        char *port = domain_end - 1;
        int t = domain_temp[(unsigned char)*port];
        uint16_t domain_len = domain_end - s;
        while (port > s && port > (domain_end - 6))
        {
            if (*port != ':')
                --port;
            else
                break;
        }
        if (*port == ':')
        {
            t = domain_temp[(unsigned char)*(port - 1)];
            b.port = (uint16_t)strtoul(port + 1, NULL, 10);
            domain_len = port - s;
            if (s[-3] == 1 && b.port == 443)
            {
                memmove(s + 4, s, domain_len);
                s += 4;
                s[-3] = 1 & 0xff;
            }
            else if (s[-3] == 0 && b.port == 80)
            {
                memmove(s + 3, s, domain_len);
                s += 3;
                s[-3] = 0 & 0xff;
            }
        }
        b.start = s;
        b.n = domain_len - 1;
        uint16_t len = se - s - 9; // -9 +2  [-1]type:len[2]:start->end
        *(uint16_t *)(s - 2) = len - 1;
        list_domainport_[t][count[t]++] = (b);
#ifdef DEBUG
        *(se - 9) = '\0';
        printf("load:[%d,%d]%d %s\n", t, port, len, s);
#endif
        s = se + 1;
    }
    return 0;
}

UrlFilter *UrlFilter::load(char *p, uint64_t size)
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
    UrlFilter *filter = new UrlFilter();
    filter->prepare_buf(p, size);
    for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
    {
        if (filter->list_domainport_count_[i] > 0)
            filter->list_domainport_[i] = (DomainPortBuf *)malloc(filter->list_domainport_count_[i] * sizeof(DomainPortBuf));
    }
    for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
    {
        if (filter->list_count_[i] > 0)
            filter->list_[i] = (char **)malloc(filter->list_count_[i] * sizeof(char *));
    }
    filter->load_(p, size);
    filter->p_ = p;
    filter->size_ = size;
    return filter;
}

int UrlFilter::load1()
{
    int count = prepare_buf(p_, size_);
    for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
    {
        if (list_domainport_count_[i] > 0)
        {
            if (max_list_domainport_count_[i] == 0)
            {
                list_domainport_[i] = (DomainPortBuf *)malloc(list_domainport_count_[i] * sizeof(DomainPortBuf));
                max_list_domainport_count_[i] = list_domainport_count_[i];
            }
            else if (list_domainport_count_[i] > max_list_domainport_count_[i])
            {
                if (list_domainport_[i] != NULL)
                {
                    free(list_domainport_[i]);
                    list_domainport_[i] = NULL;
                }
                list_domainport_[i] = (DomainPortBuf *)malloc(list_domainport_count_[i] * sizeof(DomainPortBuf));
                max_list_domainport_count_[i] = list_domainport_count_[i];
            }
        }
    }
    for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
    {
        if (list_count_[i] > 0)
        {
            if (list_count_[i] > max_list_count_[i])
            {
                if (list_[i] != NULL)
                {
                    free(list_[i]);
                    list_[i] = NULL;
                }
                list_[i] = (char **)malloc(list_count_[i] * sizeof(char *));
                max_list_count_[i] = list_count_[i];
            }
        }
    }

    load_(p_, size_);
    return count;
}

static int prepare_buf2(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    int count = 0;
    while (s < e)
    {
        char *se = strchr(s + 3, '\n');
        ++count;
        s = se + 1;
    }
    return count;
}

inline int load2_(char *p, uint64_t size, char **list)
{
    char *e = p + size;
    char *s = p + 3;
    int count = 0;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        list[count++] = s - 2;
        s = se + 4;
    }
    return count;
}

int UrlFilter::load2(char *p, uint size, int type, int count)
{
    // int count = prepare_buf2(p, size);
    if (count + list_count_[type] > max_list_count_[type])
    {
        char **list = list_[type];
        list_[type] = (char **)malloc((count + list_count_[type]) * sizeof(char *));
        if (list_count_[type] > 0)
        {
            memcpy(list_[type], list, list_count_[type] * sizeof(char *));
        }
        if (list != NULL)
            free(list);
    }
    list_count_[type] += load2_(p, size, list_[type] + list_count_[type]);

    return count;
}

void UrlFilter::set_dp_list(vector<DomainFilter *> &list)
{
    this->list_domainfilter_ = list;
}

void UrlFilter::set_pf_list(vector<PrefixFilter *> &list)
{
    this->list_prefixfilter_ = list;
}

bool cmp_dp(const DomainPortBuf &e1, const DomainPortBuf &e2)
{
    if (e1.port < e2.port)
        return true;
    else if (e1.port > e2.port)
        return false;
    const char *pa = e1.start;
    const char *pb = e2.start;
    uint16_t na = e1.n;
    uint16_t nb = e2.n;
    int ret = cmpbuf_dp(pa, na, pb, nb);
#ifdef DEBUG
    printf("cmp_dp:ret=%d,e1:%s,%d,port=%d,e2:%s,%d,port=%d\n", ret, e1.start, na, e1.port, e2.start, nb, e2.port);
#endif
    if (ret != 0)
        return ret == -1 ? true : false;
    return na < nb;
}

bool cmp_dp_port(const DomainPortBuf &e1, const DomainPortBuf &e2)
{
    const char *pa = e1.start;
    const char *pb = e2.start;
    uint16_t na = e1.n;
    uint16_t nb = e2.n;
    int ret = cmpbuf_dp(pa, na, pb, nb);
#ifdef DEBUG
    printf("cmp_dp_port:ret=%d,e1:%s,%d,port=%d,e2:%s,%d,port=%d\n", ret, e1.start, na, e1.port, e2.start, nb, e2.port);
#endif
    if (ret != 0)
        return ret == -1 ? true : false;
    return na < nb;
}

bool cmp_dp1(const DomainPortBuf &e1, const char *e2)
{
    const char *pa = e1.start;
    const char *pb = e2;
    uint16_t na = e1.n;
    uint16_t nb = *(uint16_t *)(pb - 2);
    int ret = cmpbuf_dp(pa, na, pb, nb);
#ifdef DEBUG
    printf("cmp_dp:ret=%d,e1:%s,%d,port=%d,e2:%s,%d,port=%d\n", ret, e1.start, na, e1.port, e2, nb);
#endif
    if (ret != 0)
        return ret == -1 ? true : false;
    return na < nb;
}

int filter_domainport_(const DomainPortBuf &in,
                       const vector<DomainPortBuf>::iterator start,
                       const vector<DomainPortBuf>::iterator end,
                       stDPRES &res)
{
    res.ret = -1;
    if (start == end)
        return -1;
    vector<DomainPortBuf>::iterator iter = upper_bound(start, end, in, cmp_dp);
#ifdef DEBUG
    printf("start=%p,end=%p,res=%p\n", start, end, iter);
#endif
    --iter;

    while (iter >= start)
    {
        const char *pa = iter->start;
        const char *pb = in.start;
        uint16_t na = iter->n;
        uint16_t nb = in.n;
        uint16_t porta = iter->port;
        uint16_t portb = in.port;
        int ret = cmpbuf_dp(pa, na, pb, nb);
#ifdef DEBUG
        printf("filter_domainport_:ret=%d,in:%s,na=%d,porta=%d;iter:%s,nb=%d,portb=%d, hit=%d\n", ret, in.start, nb, portb, iter->start, na, porta, (int)(iter->hit));
#endif
        if (ret != 0)
        {
            return -1;
        }
        else
        {
            if (porta == 0 || porta == portb)
            {
                if (na == nb || ((na < nb) && (pb[nb - na - 1] == '.' || pa[0] == '.')))
                {
                    res.n = na;
                    res.port = porta;
                    res.ret = (int)iter->hit;
                    printf("filter_domainport_:ret=%d\n", res.ret);
                    return (int)(iter->hit);
                }
                else
                {
                    --iter;
                    continue;
                }
            }
            else
            {
                --iter;
                continue;
            }
            /*
            if (porta == 0 || porta == portb)
            {
                if (na > nb)
                {
                    --iter;
                    continue;
                } else if (na < nb)
                {
                    if (pb[nb-na-1]=='.')
                    {
                        res = iter;
                        return (int)(res->allow);
                    }
                    else
                    {
                        --iter;
                        continue;
                    }
                }
                else
                {
                    res = iter;
                    return (int)(res->allow);
                }
            }
            else
            {
                --iter;
                continue;
            }
            */
        }
    };
    return -1;
}

struct stDP_Port_CMP
{
    uint16_t n;
    char c;
};

inline bool cmp_dp_len(const char *pa, const uint16_t na, const char *pb, const uint16_t nb)
{
    return (na == nb || (na > nb && (pa[na - nb - 1] == '.' || pb[0] == '.')));
}

bool cmp_dp1_loop(const stDP_Port_CMP &e1, const char *e2)
{
    uint16_t nb = *(uint16_t *)(e2 - 2);
    if (e1.n > nb)
        return false;
    return e1.c < e2[nb - e1.n];
}

int filter_domainport_impl(const DomainPortBuf &in,
                           char **start,
                           const int size,
                           const int *range)
{
    if (size == 0)
        return -1;
    char **iter = upper_bound(start, start + size, in, cmp_dp1);
    if (iter == start)
    {
        return -1;
    }
    --iter;
    char *pa = in.start;
    uint16_t na = in.n;
    int count = range[iter - start];
    //cmp the last one
    const char *pb = *iter;
    uint16_t nb = *(uint16_t *)(pb - 2);
    int ret = cmpbuf_dp(pa, na, pb, nb);

    if (ret == 0 && (na == nb || (na > nb && (pa[na - nb - 1] == '.' || pb[0] == '.'))))
    {
        return (int)nb;
    }
    else
    {
        --iter;
        --count;
    }
    if (count < 3)
    {
        while (count > 0)
        {
            pb = *iter;
            nb = *(uint16_t *)(pb - 2);
            ret = cmpbuf_dp(pa, na, pb, nb);

            if (ret == 0 && (na == nb || (na > nb && (pa[na - nb - 1] == '.' || pb[0] == '.'))))
            {
                return (int)nb;
            }
            else
            {
                --iter;
                --count;
            }
        }
    }
    else
    {
        //cmp the first one
        char **p = iter + 1 - count;
        pb = p[0];
        nb = *(uint16_t *)(pb - 2);
        if (na > nb)
        {
            ret = cmpbuf_dp(pa, na, pb, nb);
            if (ret == 0 && cmp_dp_len(pa, na, pb, nb))
            {
                int final = (int)nb;
                uint16_t count = nb + 1;
                p += 1;
                stDP_Port_CMP s1 = {count, pa[na - count]};
                while (count <= na)
                {
                    char **p1 = upper_bound(p, iter + 1, s1, cmp_dp1_loop);
                    if (p1 > iter)
                    {
                        break;
                    }
                    pb = *p1;
                    nb = *(uint16_t *)(pb - 2);
                    ret = cmpbuf_dp(pa, na - count, pb, nb - count);
                    if (ret == 0 && cmp_dp_len(pa, na, pb, nb))
                    {
                        final = (int)nb;
                    }
                    if (p1 == iter)
                    {
                        break;
                    }
                    ++count;
                    p = p1 + 1;
                    ++s1.n;
                    s1.c = pa[na - count];
                }
                return final;
            }
        }
    }
    return -1;
}

bool cmp_dp_port_loop(const stDP_Port_CMP &e1, const DomainPortBuf &e2)
{
    if (e1.n > e2.n)
        return false;
    return e1.c < e2.start[e2.n - e1.n];
}

int filter_port_impl(DomainPortBuf in,
                     DomainPortBuf *start,
                     const int size,
                     const int *range)
{
    if (size == 0)
        return -1;
    in.n += 1;
    DomainPortBuf *iter = upper_bound(start, start + size, in, cmp_dp_port);
    if (iter == start)
    {
        return -1;
    }
    --iter;
    char *pa = in.start;
    uint16_t na = in.n;
    int count = range[iter - start];
    const char *pb = iter->start;
    uint16_t nb = iter->n;
    int ret = cmpbuf_dp(pa, na, pb, nb);

    if (ret == 0 && cmp_dp_len(pa, na, pb, nb))
    {
        return (int)nb;
    }
    else
    {
        --iter;
        --count;
    }
    if (count < 3)
    {
        while (count > 0)
        {
            pb = iter->start;
            nb = iter->n;
            ret = cmpbuf_dp(pa, na, pb, nb);

            if (ret == 0 && cmp_dp_len(pa, na, pb, nb))
            {
                return (int)nb;
            }
            else
            {
                --iter;
                --count;
            }
        }
    }
    else
    {
        DomainPortBuf *p = iter + 1 - count;
        pb = p->start;
        nb = p->n;
        if (na > nb)
        {
            ret = cmpbuf_dp(pa, na, pb, nb);
            if (ret == 0 && cmp_dp_len(pa, na, pb, nb))
            {
                int final = (int)nb;
                uint16_t count = nb + 1;
                p += 1;
                stDP_Port_CMP s1 = {count, pa[na - count]};
                while (count <= na)
                {
                    DomainPortBuf *p1 = upper_bound(p, iter + 1, s1, cmp_dp_port_loop);
                    if (p1 > iter)
                    {
                        break;
                    }
                    pb = p1->start;
                    nb = p1->n;
                    ret = cmpbuf_dp(pa, na - count, pb, nb - count);
                    if (ret == 0 && cmp_dp_len(pa, na, pb, nb))
                    {
                        final = (int)nb;
                    }
                    if (p1 == iter)
                    {
                        break;
                    }
                    ++count;
                    p = p1 + 1;
                    ++s1.n;
                    s1.c = pa[na - count];
                }
                return final;
            }
        }
    }
    return -1;
}

int filter_domainport_1(const DomainPortBuf &in,
                        const DomainFilterMerge *filter,
                        int t,
                        stDPRES &res)
{
    res = {0, 0, -1};
    DomainPortBuf *port_start = filter->port_start_[1][in.port];
    int size = filter->port_size_[1][in.port];
    int n = filter_port_impl(in, port_start, size, filter->port_range_[1][in.port]);
    if (n > 0)
    {
        res.n = n;
        res.ret = 1;
        res.port = in.port;
        if (n == in.n + 1)
            return res.ret;
    }
    port_start = filter->port_start_[0][in.port];
    size = filter->port_size_[0][in.port];
    n = filter_port_impl(in, port_start, size, filter->port_range_[0][in.port]);
    if (n > 0 && n > res.n)
    {
        res.n = n;
        res.ret = 0;
        res.port = in.port;
        if (n == in.n + 1)
            return res.ret;
    }
    if (res.n != 0)
        return res.ret;
    char **start = filter->list_[1][t];
    size = filter->list_count_[1][t];
    n = filter_domainport_impl(in, start, size, filter->list_range_[1][t]);
    if (n > 0)
    {
        res.n = n;
        res.ret = 1;
        res.port = 0;
        if (n == in.n)
            return res.ret;
    }
    start = filter->list_[0][t];
    size = filter->list_count_[0][t];
    n = filter_domainport_impl(in, start, size, filter->list_range_[0][t]);
    if (n > 0 && n > res.n)
    {
        res.n = n;
        res.ret = 0;
        res.port = 0;
        if (n == in.n)
            return res.ret;
    }
    if (res.ret > -1)
    {
        if (filter->list_c[1][t] > 0)
        {
            res.n = 1;
            res.ret = 1;
            res.port = 0;
            return res.ret;
        }
        else if (filter->list_c[0][t] > 0)
        {
            res.n = 1;
            res.ret = 0;
            res.port = 0;
            return res.ret;
        }
    }

    return res.ret;
}

bool cmp_dpres(const pair<int, vector<DomainPortBuf>::iterator> &e1, const pair<int, vector<DomainPortBuf>::iterator> &e2)
{
    if (e2.first == -1)
        return false;
    if (e1.first == -1)
        return true;
    int sub = e1.second->port - e2.second->port;
    if (sub < 0)
        return true;
    else if (sub > 0)
        return false;
    else
    {
        sub = e1.second->n - e2.second->n;
        if (sub < 0)
            return true;
        else if (sub > 0)
            return false;
        else
        {
            return e1.first < e2.first;
        }
    }
}

void UrlFilter::filter_domainport()
{
    stDPRES res, output;
    int count[DOMAIN_CHAR_COUNT] = {0};
    DomainFilterMerge *filter = (DomainFilterMerge *)(this->domainfilter_);
    for (int t = 0; t < DOMAIN_CHAR_COUNT; ++t)
    {
        for (int i = 0; i < list_domainport_count_[t]; ++i)
        {
            DomainPortBuf &in = list_domainport_[t][i];
            res = {0, 0, -1};
            // for (int j = list_domainfilter_.size() - 1; j < list_domainfilter_.size(); ++j)
            {
                if (filter_domainport_1(in, filter, t, output) != -1)
                {
                    if (output.n > res.n)
                    {
                        res = output;
                    }
                    else if (output.n == res.n)
                    {
                        if (output.port > res.port)
                            res = output;
                        else if (output.port == res.port && output.ret > res.ret)
                            res = output;
                    }
                }
            }
            if (res.ret == 1) // -
            {
                counters_.hit++;
                uint32_t tag = (uint32_t)strtoul(in.start + (*((uint16_t *)(in.start - 2))) + 1 + 1, NULL, 16);
                counters_.hitchecksum ^= tag;
            }
            else
            {
                if (res.ret == -1) //miss
                {
                    in.start[-3] |= 0x40;
                }
                arrangesuffix(in.start + 1, *(uint16_t *)(in.start - 2));
                int t1 = domain_temp[(unsigned char)*in.start];
                list_[t1][count[t1]++] = (in.start - 2);
#ifdef DEBUG
                printf("ret=%d,urlfilter:%s\n", res.ret, in.start);
#endif
            }
        }
    }
    for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
        list_count_[i] = count[i];
}

bool cmp_pf(const char *e1, const char *e2)
{
    const char *pa = e1;
    const char *pb = e2;
    int na = (int)*((uint16_t *)pa);
    int nb = (int)*((uint16_t *)pb);
    pa += 2 + 1;
    pb += 2 + 1;
    int ret = cmpbuf_pf(pa, na, pb, nb);
#ifdef DEBUG
    printf("cmp_pf:ret=%d,e1:%s,%d,e2:%s,%d\n", ret, e1 + 2, na, e2 + 2, nb);
#endif
    if (ret != 0)
        return ret == -1 ? true : false;
    return na <= nb;
}

int len_eq(const char *e1, const char *e2)
{
    int na = (int)*((uint16_t *)e1);
    int nb = (int)*((uint16_t *)e2);
#ifdef DEBUG
    printf("len_eq:%d\n", na - nb);
#endif
    return (na - nb);
}

int cmp_pf_eq(const char *e1, const char *e2)
{
    const char *pa = e1;
    const char *pb = e2;
    int na = (int)*((uint16_t *)pa);
    int nb = (int)*((uint16_t *)pb);
    pa += 2 + 1;
    pb += 2 + 1;
    int ret = cmpbuf_pf(pa, na, pb, nb);
#ifdef DEBUG
    printf("cmp_pf_eq:ret=%d,e1=%s,na=%d,e2=%s,nb=%d\n", ret, pa, na, pb, nb);
#endif
    return ret;
}

struct stPFRES
{
    uint16_t len;
    int type;
    int hit;
};

bool cmp_pfres(const stPFRES &e1, const stPFRES &e2)
{
    int sub = e1.len - e2.len;
    if (sub != 0)
        return sub < 0 ? true : false;
    sub = e1.type - e2.type;
    if (sub != 0)
        return sub < 0 ? true : false;
    sub = e1.hit - e2.hit;
    if (sub != 0)
        return sub < 0 ? true : false;
    return false;
}

struct stPF_CMP
{
    uint16_t die_8;
    uint16_t l8;
    uint16_t n;
    char c;
};
inline int com_locat(const stPF_CMP &s1, uint16_t nb)
{
    int ret = 0;
    if (nb >= s1.die_8)
    {
        ret = s1.die_8 - s1.l8;
    }
    else
    {
        ret = s1.die_8 - 8 + s1.l8 - 1;
    }
    return ret;
}

bool cmp_pf_loop(const stPF_CMP &e1, const char *e2)
{
    uint16_t nb = *(uint16_t *)(e2);
    e2 += 3;
    if (e1.n > nb)
        return false;
    return e1.c < e2[com_locat(e1, nb)];
}

int filter_prefix_(const char *in, PrefixFilter *filter, bool https, int idx1, stPFRES &output)
{
    // vector<char *>::iterator start, end;
    // vector<char *>::iterator res;
    char **start;
    char **end;
    char **res;
    output = {0, -1, 0};
    const char *pa = in;
    uint16_t na = *(uint16_t *)(pa);
    pa += 3;
#ifdef DEBUG
    printf("filter_prefix_:https:%d,in:%s\n", https, in + 2);
#endif

#ifdef DEBUG
    printf("filter_prefix_:https:[%d,%d]\n", 2, 1);
#endif
    if (filter->list_count_[2][1][https][idx1] > 0)
    {
        start = filter->list_https_[2][1][https][idx1];
        end = filter->list_https_[2][1][https][idx1] + filter->list_count_[2][1][https][idx1];
        res = upper_bound(start, end, in, cmp_pf);
        if (res != end)
        {
            if (len_eq(in, *res) == 0 && cmp_pf_eq(in, *res) == 0)
            {
                output.len = 10000;
                output.type = 2;
                output.hit = 1;
                return 1;
            }
        }
    }
#ifdef DEBUG
    printf("filter_prefix_:https:[%d,%d]\n", 2, 0);
#endif
    if (filter->list_count_[2][0][https][idx1] > 0)
    {
        start = filter->list_https_[2][0][https][idx1];
        end = filter->list_https_[2][0][https][idx1] + filter->list_count_[2][0][https][idx1];
        res = upper_bound(start, end, in, cmp_pf);
        if (res != end)
        {
            if (len_eq(in, *res) == 0 && cmp_pf_eq(in, *res) == 0)
            {
                output.len = 10000;
                output.type = 2;
                output.hit = 0;
                return 0;
            }
        }
    }
#ifdef DEBUG
    printf("filter_prefix_:https:[%d,%d]\n", 1, 1);
#endif
    if (filter->list_count_[1][1][https][idx1] > 0)
    {
        start = filter->list_https_[1][1][https][idx1];
        end = filter->list_https_[1][1][https][idx1] + filter->list_count_[1][1][https][idx1];
        res = upper_bound(start, end, in, cmp_pf);
#ifdef DEBUG
        printf("start=%p,end=%p,res=%p\n", start, end, res);
#endif
        --res;
        if (res >= start)
        {
            const char *pb = *res;
            uint16_t nb = *(uint16_t *)(pb);
            if (na > nb && cmpbuf_pf(pa, na, pb + 3, nb) == 0)
            {
                output.len = nb;
                output.type = 1;
                output.hit = 1;
            }
            else
            {
                --res;
                if (res >= start)
                {
                    int count = filter->list_range_[1][1][https][idx1][res - start];
                    if (count < 3)
                    {
                        while (count > 0)
                        {
                            pb = *res;
                            nb = *(uint16_t *)(pb);
                            if (na > nb && cmpbuf_pf(pa, na, pb + 3, nb) == 0)
                            {
                                output.len = nb;
                                output.type = 1;
                                output.hit = 1;
                                break;
                            }
                            --res;
                            --count;
                        }
                    }
                    else
                    {
                        char **p = res + 1 - count;
                        pb = *p;
                        nb = *(uint16_t *)(pb);
                        if (na > nb)
                        {
                            int ret = cmpbuf_pf(pa, na, pb + 3, nb);
                            if (ret == 0)
                            {
                                int final = (int)nb;
                                uint16_t count = nb + 1;
                                p += 1;
                                stPF_CMP s1;
                                s1.n = count;
                                s1.die_8 = ((int)ceil((double)s1.n / 8)) * 8;
                                s1.l8 = s1.n % 8 == 0 ? 8 : s1.n % 8;
                                s1.c = pa[com_locat(s1, na)];
                                while (count < na)
                                {
                                    char **p1 = upper_bound(p, res + 1, s1, cmp_pf_loop);
                                    if (p1 > res)
                                    {
                                        break;
                                    }
                                    pb = *p1;
                                    nb = *(uint16_t *)(pb);
                                    int offset = ((int)(count / 8)) * 8;
                                    ret = cmpbuf_pf(pa, na - offset, pb, nb - offset);
                                    if (ret == 0)
                                    {
                                        final = (int)nb;
                                    }
                                    if (p1 == res)
                                    {
                                        break;
                                    }
                                    ++count;
                                    p = p1 + 1;
                                    ++s1.n;
                                    s1.die_8 = ((int)ceil((double)s1.n / 8)) * 8;
                                    s1.l8 = s1.n % 8 == 0 ? 8 : s1.n % 8;
                                    s1.c = pa[com_locat(s1, na)];
                                }
                                output.len = final;
                                output.type = 1;
                                output.hit = 1;
                            }
                        }
                    }
                }
            }
        }
    }
#ifdef DEBUG
    printf("filter_prefix_:https:[%d,%d]\n", 1, 0);
#endif
    if (filter->list_count_[1][0][https][idx1] > 0)
    {
        start = filter->list_https_[1][0][https][idx1];
        end = filter->list_https_[1][0][https][idx1] + filter->list_count_[1][0][https][idx1];
#ifdef DEBUG
        printf("start=%p,end=%p,res=%p\n", start, end, res);
#endif
        res = upper_bound(start, end, in, cmp_pf);
        --res;
        if (res >= start)
        {
            const char *pb = *res;
            uint16_t nb = *(uint16_t *)(pb);
            if (na > nb && cmpbuf_pf(pa, na, pb + 3, nb) == 0)
            {
                if (nb > output.len)
                {
                    output.len = nb;
                    output.type = 1;
                    output.hit = 0;
                }
            }
            else
            {
                --res;
                if (res >= start)
                {
                    int count = filter->list_range_[1][0][https][idx1][res - start];
                    if (count < 3)
                    {
                        while (count > 0)
                        {
                            pb = *res;
                            nb = *(uint16_t *)(pb);
                            if (na > nb && cmpbuf_pf(pa, na, pb + 3, nb) == 0)
                            {
                                if (nb > output.len)
                                {
                                    output.len = nb;
                                    output.type = 1;
                                    output.hit = 0;
                                }
                                break;
                            }
                            --res;
                            --count;
                        }
                    }
                    else
                    {
                        char **p = res + 1 - count;
                        pb = *p;
                        nb = *(uint16_t *)(pb);
                        if (na > nb)
                        {
                            int ret = cmpbuf_pf(pa, na, pb + 3, nb);
                            if (ret == 0)
                            {
                                int final = (int)nb;
                                uint16_t count = nb + 1;
                                p += 1;
                                stPF_CMP s1;
                                s1.n = count;
                                s1.die_8 = ((int)ceil((double)s1.n / 8)) * 8;
                                s1.l8 = s1.n % 8 == 0 ? 8 : s1.n % 8;
                                s1.c = pa[com_locat(s1, na)];
                                while (count < na)
                                {
                                    char **p1 = upper_bound(p, res + 1, s1, cmp_pf_loop);
                                    if (p1 > res)
                                    {
                                        break;
                                    }
                                    pb = *p1;
                                    nb = *(uint16_t *)(pb);
                                    int offset = ((int)(count / 8)) * 8;
                                    ret = cmpbuf_pf(pa, na - offset, pb, nb - offset);
                                    if (ret == 0)
                                    {
                                        final = (int)nb;
                                    }
                                    if (p1 == res)
                                    {
                                        break;
                                    }
                                    ++count;
                                    p = p1 + 1;
                                    ++s1.n;
                                    s1.die_8 = ((int)ceil((double)s1.n / 8)) * 8;
                                    s1.l8 = s1.n % 8 == 0 ? 8 : s1.n % 8;
                                    s1.c = pa[com_locat(s1, na)];
                                }
                                if (final > output.len)
                                {
                                    output.len = final;
                                    output.type = 1;
                                    output.hit = 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
#ifdef DEBUG
    printf("filter_prefix_:https:[%d,%d]\n", 0, 1);
#endif
    if (filter->list_count_[0][1][https][idx1] > 0)
    {
        start = filter->list_https_[0][1][https][idx1];
        end = filter->list_https_[0][1][https][idx1] + filter->list_count_[0][1][https][idx1];
        res = upper_bound(start, end, in, cmp_pf);
        if (res != end)
        {
            const char *pb = *res;
            uint16_t nb = *(uint16_t *)(pb);
            if (na == nb && cmpbuf_pf(pa, na, pb + 3, nb) == 0)
            {
                output.len = nb;
                output.type = 0;
                output.hit = 1;
                return 1;
            }
        }
        --res;
        if (res >= start)
        {
            const char *pb = *res;
            uint16_t nb = *(uint16_t *)(pb);
            if (na > nb && cmpbuf_pf(pa, na, pb + 3, nb) == 0)
            {
                if (nb > output.len)
                {
                    output.len = nb;
                    output.type = 0;
                    output.hit = 1;
                }
            }
            else
            {
                --res;
                if (res >= start)
                {
                    int count = filter->list_range_[0][1][https][idx1][res - start];
                    if (count < 3)
                    {
                        while (count > 0)
                        {
                            pb = *res;
                            nb = *(uint16_t *)(pb);
                            if (na > nb && cmpbuf_pf(pa, na, pb + 3, nb) == 0)
                            {
                                if (nb > output.len)
                                {
                                    output.len = nb;
                                    output.type = 0;
                                    output.hit = 1;
                                }
                                break;
                            }
                            --res;
                            --count;
                        }
                    }
                    else
                    {
                        char **p = res + 1 - count;
                        pb = *p;
                        nb = *(uint16_t *)(pb);
                        if (na > nb)
                        {
                            int ret = cmpbuf_pf(pa, na, pb + 3, nb);
                            if (ret == 0)
                            {
                                int final = (int)nb;
                                uint16_t count = nb + 1;
                                p += 1;
                                stPF_CMP s1;
                                s1.n = count;
                                s1.die_8 = ((int)ceil((double)s1.n / 8)) * 8;
                                s1.l8 = s1.n % 8 == 0 ? 8 : s1.n % 8;
                                s1.c = pa[com_locat(s1, na)];
                                while (count < na)
                                {
                                    char **p1 = upper_bound(p, res + 1, s1, cmp_pf_loop);
                                    if (p1 > res)
                                    {
                                        break;
                                    }
                                    pb = *p1;
                                    nb = *(uint16_t *)(pb);
                                    int offset = ((int)(count / 8)) * 8;
                                    ret = cmpbuf_pf(pa, na - offset, pb, nb - offset);
                                    if (ret == 0)
                                    {
                                        final = (int)nb;
                                    }
                                    if (p1 == res)
                                    {
                                        break;
                                    }
                                    ++count;
                                    p = p1 + 1;
                                    ++s1.n;
                                    s1.die_8 = ((int)ceil((double)s1.n / 8)) * 8;
                                    s1.l8 = s1.n % 8 == 0 ? 8 : s1.n % 8;
                                    s1.c = pa[com_locat(s1, na)];
                                }
                                if (final > output.len)
                                {
                                    output.len = final;
                                    output.type = 0;
                                    output.hit = 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
#ifdef DEBUG
    printf("filter_prefix_:https:[%d,%d]\n", 0, 0);
#endif
    if (filter->list_count_[0][0][https][idx1] > 0)
    {
        start = filter->list_https_[0][0][https][idx1];
        end = filter->list_https_[0][0][https][idx1] + filter->list_count_[0][0][https][idx1];
        res = upper_bound(start, end, in, cmp_pf);
#ifdef DEBUG
        printf("start=%p,end=%p,res=%p\n", start, end, res);
#endif
        if (res != end)
        {
            const char *pb = *res;
            uint16_t nb = *(uint16_t *)(pb);
            if (na == nb && cmpbuf_pf(pa, na, pb + 3, nb) == 0)
            {
                output.len = nb;
                output.type = 0;
                output.hit = 0;
                return 0;
            }
        }
        --res;

        if (res >= start)
        {
            const char *pb = *res;
            uint16_t nb = *(uint16_t *)(pb);
            if (na > nb && cmpbuf_pf(pa, na, pb + 3, nb) == 0)
            {
                if (nb > output.len)
                {
                    output.len = nb;
                    output.type = 0;
                    output.hit = 0;
                }
            }
            else
            {
                --res;
                if (res >= start)
                {
                    int count = filter->list_range_[0][0][https][idx1][res - start];
                    if (count < 3)
                    {
                        while (count > 0)
                        {
                            pb = *res;
                            nb = *(uint16_t *)(pb);
                            if (na > nb && cmpbuf_pf(pa, na, pb + 3, nb) == 0)
                            {
                                if (nb > output.len)
                                {
                                    output.len = nb;
                                    output.type = 0;
                                    output.hit = 0;
                                }
                                break;
                            }
                            --res;
                            --count;
                        }
                    }
                    else
                    {
                        char **p = res + 1 - count;
                        pb = *p;
                        nb = *(uint16_t *)(pb);
                        if (na > nb)
                        {
                            int ret = cmpbuf_pf(pa, na, pb + 3, nb);
                            if (ret == 0)
                            {
                                int final = (int)nb;
                                uint16_t count = nb + 1;
                                p += 1;
                                stPF_CMP s1;
                                s1.n = count;
                                s1.die_8 = ((int)ceil((double)s1.n / 8)) * 8;
                                s1.l8 = s1.n % 8 == 0 ? 8 : s1.n % 8;
                                s1.c = pa[com_locat(s1, na)];
                                while (count < na)
                                {
                                    char **p1 = upper_bound(p, res + 1, s1, cmp_pf_loop);
                                    if (p1 > res)
                                    {
                                        break;
                                    }
                                    pb = *p1;
                                    nb = *(uint16_t *)(pb);
                                    int offset = ((int)(count / 8)) * 8;
                                    ret = cmpbuf_pf(pa, na - offset, pb, nb - offset);
                                    if (ret == 0)
                                    {
                                        final = (int)nb;
                                    }
                                    if (p1 == res)
                                    {
                                        break;
                                    }
                                    ++count;
                                    p = p1 + 1;
                                    ++s1.n;
                                    s1.die_8 = ((int)ceil((double)s1.n / 8)) * 8;
                                    s1.l8 = s1.n % 8 == 0 ? 8 : s1.n % 8;
                                    s1.c = pa[com_locat(s1, na)];
                                }
                                if (final > output.len)
                                {
                                    output.len = final;
                                    output.type = 0;
                                    output.hit = 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return output.hit;
}

void UrlFilter::filter_prefix()
{
    stPFRES output, res;
    PrefixFilter *filter = this->prefixfilter_;
    for (int k = 0; k < DOMAIN_CHAR_COUNT; ++k)
    {
        for (int i = 0; i < list_count_[k]; ++i)
        {
            res = {0, -1, 0};
            const char *in = list_[k][i];
            bool https = (in[-1] & 0x0f) == 1;
            // for (int j = list_prefixfilter_.size() - 1; j < list_prefixfilter_.size(); ++j)
            {
                filter_prefix_(in, filter, https, k, output);
                // printf("filter_prefix end:https:%d,in:%s\n", https, in + 2);
                if (cmp_pfres(res, output))
                {
                    res = output;
                }
            }
#ifdef DEBUG
            printf("filter_prefix:https:%d,in:%s,len:%d,type:%d,hit:%d\n", ((in[-1] & 0x0f) == 1), in + 2, res.len, res.type, res.hit);
#endif
            const char *tagbuf = in + (*((uint16_t *)(in))) + 3 + 1;
            // printf("tagbuf:%s\n", tagbuf);
            uint32_t tag = (uint32_t)strtoul(tagbuf, NULL, 16);
            if (res.hit == 1)
            {
                counters_.hit++;

                counters_.hitchecksum ^= tag;
            }
            else
            {

#ifdef DEBUG
                printf("output->type=%d,in[-1]=%d\n", res.type, in[-1]);
#endif
                if (res.type == -1 && int(in[-1]) > 1)
                {
                    counters_.miss++;
                }
                counters_.pass++;
                counters_.passchecksum ^= tag;
                memcpy(out_ + out_offset_, tagbuf, 9);
                out_offset_ += 9;
            }
        }
    }
}

void UrlFilter::prepare_prefix()
{
    for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
        out_size_ += list_count_[i] * 9;
    if (out_size_ > 0)
        out_ = (char *)malloc(out_size_);
    out_offset_ = 0;
}

int UrlFilter::write_tag(FILE *fp)
{
    int ret = 0;
    if (out_offset_ > 0)
        ret = fwrite(out_, out_offset_, 1, fp);
    if (out_ != NULL)
    {
        free(out_);
        out_ = NULL;
    }
    out_size_ = 0;
    out_offset_ = 0;
    return ret;
}

void UrlFilter::clear_para()
{
    memset(list_count_, 0, DOMAIN_CHAR_COUNT * sizeof(int));
    memset(list_domainport_count_, 0, DOMAIN_CHAR_COUNT * sizeof(int));
    size_ = 0;
}

void UrlFilter::clear_counter()
{
    counters_.hit = 0;
    counters_.pass = 0;
    counters_.miss = 0;
    counters_.hitchecksum = 0;
    counters_.passchecksum = 0;
}

void UrlFilter::clear_domain_list()
{
    for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
    {
        if (list_domainport_[i] != NULL)
        {
            free(list_domainport_[i]);
            list_domainport_[i] = NULL;
        }
    }
    memset(list_domainport_count_, 0, DOMAIN_CHAR_COUNT * sizeof(int));
    memset(max_list_domainport_count_, 0, DOMAIN_CHAR_COUNT * sizeof(int));
}
