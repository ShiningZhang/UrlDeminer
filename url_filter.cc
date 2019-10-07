#include "url_filter.h"

#include <string.h>
#include <string>

#include<algorithm>
#include <utility>

#include "domain_filter.h"
#include "prefix_filter.h"

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
        char *port = domain_end;
        uint16_t domain_len = domain_end - s;
        while(port > s && port > (domain_end - 6))
        {
            if (*port != ':')
                --port;
            else
                break;
        }
        if (*port == ':')
        {
            b.port = (uint16_t)strtoul(port+1, NULL, 10);
            domain_len=  port - s;
            if (s[-3] == 1 && b.port = 443)
            {
                memmove(s+4,s,domain_len);
                s +=4;
                s[-3] = 1 & 0xff;
            } else if (s[-3] == 0 && b.port = 80)
            {
                memmove(s+3,s,domain_len);
                s +=3;
                s[-3] = 0 & 0xff;
            }
        }
        b.start = s;
        b.n = domain_len;
        uint16_t len = se - s -6; // -9 +2  [-1]type:len[2]:start->end
        *(uint16_t *)(s-2) = len;
        list_domainport_.push_back(b);
        s = se + 1;
    }
}

UrlFilter *UrlFilter::load(char *p, uint64_t size)
{
    UrlFilter *filter = new UrlFilter();
    filter->load_(p, size);
    filter->p_ = p;
    filter->size_ = size;
    return filter;
}

void UrlFilter::set_dp_list(vector<DomainFilter *> &list)
{
    this->list_domainfilter_ = list;
}

void UrlFilter::set_pf_list(vector<PrefixFilter *> &list)
{
    this->list_prefixfilter_ = list;
}

bool cmp_dp(const DomainPortBuf& e1, const DomainPortBuf& e2)
{
    const char *pa = e1.start;
    const char *pb = e2.start;
    uint16_t na = e1.n;
    uint16_t nb = e2.n;
    int ret = cmpbuf_dp(pa, na, pb, nb);
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
    return false;
}

int filter_domainport_(const DomainPortBuf& in,
                                          const vector<DomainPortBuf>::Iterator start,
                                          const vector<DomainPortBuf>::Iterator end,
                                          vector<DomainPortBuf>::Iterator &res)
{
    res = upper_bound(start, end, in, cmp_dp);
    vector<DomainPortBuf>::Iterator iter = res - 1;
    do
    {
        const char *pa = iter->start;
        const char *pb = in.start;
        uint16_t na = iter->n;
        uint16_t nb = in.n;
        uint16_t porta = iter->port;
        uint16_t portb = in.port;
        int ret = cmpbuf_dp(pa, na, pb, nb);
        if (ret == -1)
        {
            return -1;
        }
        else if (ret == 1)
        {
            --iter;
            continue;
        }
        else
        {
            if (porta == 0 || porta == portb)
            {
                if (na == nb || ((na < nb) && (pb[nb-na-1]=='.' || pa[0] == '.')))
                {
                    res = iter;
                    return (int)(res->hit);
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
    }while(iter >= start)
}

bool cmp_dpres(const pair<int,vector<DomainPortBuf>::Iterator> &e1, const pair<int,vector<DomainPortBuf>::Iterator> &e2)
{
    if (e2.first == -1)
        return false;
    if (e1.first == -1)
        return true;
    int sub = e1.second->port - e2.second->port;
    if (sub < 0)
        return true;
    else if(sub > 0)
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
    list_.reserve(list_domainfilter_.size());
    vector<vector<DomainPortBuf>::Iterator> list_iter_start;
    vector<vector<DomainPortBuf>::Iterator> list_iter_end;
    vector<pair<int,vector<DomainPortBuf>::Iterator>> list_iter_res;
    int ret = -1;
    vector<DomainPortBuf>::Iterator iter;
    pair<int,vector<DomainPortBuf>::Iterator> max;
    for (int j = 0; j < list_domainfilter_.size(); ++j)
    {
        list_iter_start.push_back(list_domainfilter_.begin());
        list_iter_end.push_back(list_domainfilter_.end());
    }
    list_iter_res.resize(list_domainfilter_.size());
    for (int i = 0; i < list_domainport_.size(); ++i)
    {
        DomainPortBuf & in = list_domainport_[i];
        for (int j = 0; j < list_domainfilter_.size(); ++j)
        {
            ret = filter_domainport_(in, list_iter_start[j], list_iter_end[j], iter);
            list_iter_res[j].first = ret;
            list_iter_res[j].second = iter;
        }
        max = max_element(list_iter_res.begin(), list_iter_res.end(), cmp_dpres);
        /*
        for (int j = 0; j < list_domainfilter_.size(); ++j)
        {
            if (list_iter_res[j].first == -1)
            {
                continue;
            }
            else if(list_iter_res[max].first == -1)
            {
                max = j;
            }
            else
            {
                if (list_iter_res[max].second->port > list_iter_res[j].second->port)
                {
                    continue;
                }
                else if (list_iter_res[max].second->port < list_iter_res[j].second->port)
                {
                    max = j;
                    continue;
                }
                else
                {
                    if (list_iter_res[max].second->n < list_iter_res[j].second->n)
                    {
                        max = j;
                        continue;
                    }
                    else if(list_iter_res[max].second->n == list_iter_res[j].second->n)
                    {
                        if (list_iter_res[max].first > list_iter_res[j].first)
                        {
                            max = j;
                        }
                    }
                }
            }
        }
        ret = list_iter_res[max].first;
        */
        ret = max.first;
        if (ret == 1) // -
        {
            counters_.hit++;
            uint32_t tag = (uint32_t)strtoul(in.start + (*((uint16_t*)(in.start-2)))  - 2, NULL, 16);
            counters_.hitchecksum ^= tag;
        }
        else
        {
            if (ret == -1) //miss
            {
                in.start[-3] |= 0x40;
            }
            list_.push_back(in.start-2);
        }
    }
}

bool cmp_pf(const char *e1, const char *e2)
{
    const char *pa = e1;
    const char *pb = e2;
    int na = (int)*((uint16_t*)pa) - 3;
    int nb = (int)*((uint16_t*)pb) - 3;
    pa +=2;
    pb+=2;
    int ret = cmpbuf_pf(pa, na, pb, nb);
    if (ret != 0)
        return ret == -1 ? true : false;
    return na <= nb;
}

int cmp_pf_eq(const char *e1, const char *e2, int &sub)
{
    const char *pa = e1;
    const char *pb = e2;
    int na = (int)*((uint16_t*)pa) - 3;
    int nb = (int)*((uint16_t*)pb) - 3;
    sub = na - nb;
    pa +=2;
    pb+=2;
    int ret = cmpbuf_pf(pa, na, pb, nb);
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
        return sub < 0 ? true:false;
    sub = e1.type - e2.type;
    if (sub != 0)
        return sub < 0 ? true:false;
    sub = e1.hit - e2.hit;
    if (sub != 0)
        return sub < 0 ? true:false;
    return false;
}

int filter_prefix_(const char *in, const PrefixFilter* filter, bool https, stPFRES &output)
{
    vector<char*>::Iterator start, end;
    vector<char*>::Iterator res;
    vector<stPFRES> local_res;
    int sub = 0;
    if (https)
    {
        start = filter->list_https_[2][1].begin();
        end = filter->list_https_[2][1].end();
        if(binary_search(start, end, in, cmp_bf))
        {
            output.len = 10000;
            output.type = 2;
            output.hit = 1;
            return 1;
        }
        start = filter->list_https_[2][0].begin();
        end = filter->list_https_[2][0].end();
        if(binary_search(start, end, in, cmp_bf))
        {
            output.len = 10000;
            output.type = 2;
            output.hit = 0;
            return 0;
        }
        start = filter->list_https_[1][1].begin();
        end = filter->list_https_[1][1].end();
        res = upper_bound(start, end, in, cmp_bf);
        if (res != end)
        {
            while(res > start)
            {
                --res;
                int ret = cmp_pf_eq(in, *res, sub);
                if (ret != 0)
                {
                    break;
                }
                else if (sub > 0)
                {
                    output.len = (int)*((uint16_t*)(*res));
                    output.type = 1;
                    output.hit = 1;
                    local_res.emplace_back(output);
                    break;
                }
            }
        }
        start = filter->list_https_[1][0].begin();
        end = filter->list_https_[1][0].end();
        res = upper_bound(start, end, in, cmp_bf);
        if (res != end)
        {
            while(res > start)
            {
                --res;
                int ret = cmp_pf_eq(in, *res, sub);
                if (ret != 0)
                {
                    break;
                }
                else if (sub > 0)
                {
                    output.len = (int)*((uint16_t*)(*res));
                    output.type = 1;
                    output.hit = 0;
                    local_res.emplace_back(output);
                    break;
                }
            }
        }
        start = filter->list_https_[0][1].begin();
        end = filter->list_https_[0][1].end();
        res = upper_bound(start, end, in, cmp_bf);
        if (res != end)
        {
            while(res > start)
            {
                --res;
                int ret = cmp_pf_eq(in, *res, sub);
                if (ret != 0)
                {
                    break;
                }
                else if (sub == 0)
                {
                    output.len = (int)*((uint16_t*)(*res));
                    output.type = 0;
                    output.hit = 1;
                    return 1;
                }
                else if (sub > 0)
                {
                    output.len = (int)*((uint16_t*)(*res));
                    output.type = 0;
                    output.hit = 1;
                    local_res.emplace_back(output);
                    break;
                }
            }
        }
        start = filter->list_https_[0][0].begin();
        end = filter->list_https_[0][0].end();
        res = upper_bound(start, end, in, cmp_bf);
        if (res != end)
        {
            while(res > start)
            {
                --res;
                int ret = cmp_pf_eq(in, *res, sub);
                if (ret != 0)
                {
                    break;
                }
                else if (sub == 0)
                {
                    output.len = (int)*((uint16_t*)(*res));
                    output.type = 0;
                    output.hit = 1;
                    return 1;
                }
                else if (sub > 0)
                {
                    output.len = (int)*((uint16_t*)(*res));
                    output.type = 0;
                    output.hit = 1;
                    local_res.emplace_back(output);
                    break;
                }
            }
        }
        if (local_res.size() == 0)
        {
            output.len = 0;
            output.type = -1;
            output.hit = 0;
            return 0;
        }
        output = max_element(local_res.begin(), local_res.end(), cmp_pfres);
        return output.hit;
    }
    else
    {
        start = filter->list_http_[2][1].begin();
        end = filter->list_http_[2][1].end();
        if(binary_search(start, end, in, cmp_bf))
        {
            output.len = 10000;
            output.type = 2;
            output.hit = 1;
            return 1;
        }
        start = filter->list_http_[2][0].begin();
        end = filter->list_http_[2][0].end();
        if(binary_search(start, end, in, cmp_bf))
        {
            output.len = 10000;
            output.type = 2;
            output.hit = 0;
            return 0;
        }
        start = filter->list_http_[1][1].begin();
        end = filter->list_http_[1][1].end();
        res = upper_bound(start, end, in, cmp_bf);
        if (res != end)
        {
            while(res > start)
            {
                --res;
                int ret = cmp_pf_eq(in, *res, sub);
                if (ret != 0)
                {
                    break;
                }
                else if (sub > 0)
                {
                    output.len = (int)*((uint16_t*)(*res));
                    output.type = 1;
                    output.hit = 1;
                    local_res.emplace_back(output);
                    break;
                }
            }
        }
        start = filter->list_http_[1][0].begin();
        end = filter->list_http_[1][0].end();
        res = upper_bound(start, end, in, cmp_bf);
        if (res != end)
        {
            while(res > start)
            {
                --res;
                int ret = cmp_pf_eq(in, *res, sub);
                if (ret != 0)
                {
                    break;
                }
                else if (sub > 0)
                {
                    output.len = (int)*((uint16_t*)(*res));
                    output.type = 1;
                    output.hit = 0;
                    local_res.emplace_back(output);
                    break;
                }
            }
        }
        start = filter->list_http_[0][1].begin();
        end = filter->list_http_[0][1].end();
        res = upper_bound(start, end, in, cmp_bf);
        if (res != end)
        {
            while(res > start)
            {
                --res;
                int ret = cmp_pf_eq(in, *res, sub);
                if (ret != 0)
                {
                    break;
                }
                else if (sub == 0)
                {
                    output.len = (int)*((uint16_t*)(*res));
                    output.type = 0;
                    output.hit = 1;
                    return 1;
                }
                else if (sub > 0)
                {
                    output.len = (int)*((uint16_t*)(*res));
                    output.type = 0;
                    output.hit = 1;
                    local_res.emplace_back(output);
                    break;
                }
            }
        }
        start = filter->list_http_[0][0].begin();
        end = filter->list_http_[0][0].end();
        res = upper_bound(start, end, in, cmp_bf);
        if (res != end)
        {
            while(res > start)
            {
                --res;
                int ret = cmp_pf_eq(in, *res, sub);
                if (ret != 0)
                {
                    break;
                }
                else if (sub == 0)
                {
                    output.len = (int)*((uint16_t*)(*res));
                    output.type = 0;
                    output.hit = 1;
                    return 1;
                }
                else if (sub > 0)
                {
                    output.len = (int)*((uint16_t*)(*res));
                    output.type = 0;
                    output.hit = 1;
                    local_res.emplace_back(output);
                    break;
                }
            }
        }
        if (local_res.size() == 0)
        {
            output.len = 0;
            output.type = -1;
            output.hit = 0;
            return 0;
        }
        output = max_element(local_res.begin(), local_res.end(), cmp_pfres);
        return output.hit;
    }
}

void UrlFilter::filter_prefix()
{
    stPFRES output;
    vector<stPFRES> list_res;
    list_res.resize(list_prefixfilter_.size());
    for (int i = 0; i < list_.size(); ++i)
    {
        const char *in = list_[i];
         for (int j = 0; j < list_prefixfilter_.size(); ++j)
         {
            bool https = in[-1]&0x0f == 1;
            filter_prefix_(int, list_prefixfilter_[j], https, list_res[j]);
         }
         output = max_element(list_res.begin(), list_res.end(), cmp_pfres);
         char * tagbuf = in + (*((uint16_t*)(in)))  - 2;
         uint32_t tag = (uint32_t)strtoul(tagbuf, NULL, 16);
         if (output.hit == 1)
         {
            counters_.hit++;
            
            counters_.hitchecksum ^= tag;
         }
         else
         {
            if (output.type == -1 && in[-1] > 1)
            {
                counters_.miss++;
            }
            counters_.pass++;
            counters_.passchecksum ^= tag;
            memcpy(out_+out_offset_, tagbuf, 9);
            out_offset_ += 9;
         }
    }
}

void UrlFilter::prepare_prefix()
{
    out_size_ = list_.size() * 9;
    out_ = (char *)malloc(out_size_);
    out_offset_ = 0;
}

int UrlFilter::write_tag(FILE *fp)
{
    int ret = fwrite_unblocked(out_, out_offset_, 1, fp);
    free(out_);
    out_size_ = 0;
    out_offset_ = 0;
    return ret;
}