#pragma once
#ifndef SUPER_CODE_UTIL_H_
#define SUPER_CODE_UTIL_H_

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <sys/time.h>
#include <queue>
#include <mutex>
#include <condition_variable>

using namespace std;

// #define FILESPLITSIZE 23000000
// #define SMALLSIZE 110000
#define FILESPLITSIZE 256 * 1024 * 1024
#define SMALLSIZE 1100000000

#define SMALLFILESIZE 100 * 1024 * 1024
#define MAXSORT 8
#define VERYSMALLSIZE 1000000

#define BUFHEADSIZE 10

#define INITURLCOUNT 6000000

#define DOMAIN_CHAR_COUNT 39 //26 + 10 + 3

#ifdef DEBUG
#define LOG(format_string, ...)                                                                         \
    {                                                                                                   \
        struct timeval __val;                                                                           \
        gettimeofday(&__val, NULL);                                                                     \
        fprintf(stderr, "%ld.%03ld " format_string, __val.tv_sec, __val.tv_usec / 1000, ##__VA_ARGS__); \
    }
#else
#define LOG(format_string, ...)
#endif

class DomainFilter;
class PrefixFilter;
class UrlFilter;

struct GlobalST
{
    FILE *inputfilehandle;
    uint64_t inputfilesize;
    uint32_t filesplitsize;
    vector<DomainFilter *> domain_filter_list_;
    int inputend;
};

struct FilterCounters
{
    int pass;
    int hit;
    int miss;
    int invalidUrl;
    uint32_t passchecksum;
    uint32_t hitchecksum;
    FilterCounters();
};

struct DomainPortBuf
{
    char *start;
    uint16_t port;
    uint16_t n : 15;
    uint16_t hit : 1;
};

struct FileElement
{
    FILE *fp_;
    size_t size_;
    int idx;
};

extern uint64_t temp[9];
extern int domain_temp[128];

extern queue<UrlFilter *> gQueue;
extern queue<UrlFilter *> gQueueCache;
extern mutex gMutex;
extern condition_variable gCV;

size_t readcontent_unlocked(FILE *handle, char *p, uint64_t isize, int *end);
uint64_t sizeoffile(FILE *handle);
int setfilesplitsize(uint64_t inputfilesize, int maxsort);

// inline int cmp64val(int64_t ia, int64_t ib);
// inline int cmpbuf_dp(const char *pa, int na, const char *pb, int nb);
bool compare_dp(const DomainPortBuf &e1, const DomainPortBuf &e2);
// inline int cmpbuf_pf(const char *pa, int na, const char *pb, int nb);
bool compare_prefix(const char *e1, const char *e2);

void arrangesuffix(char *s, int len);

bool compare_dp_char(const char *pa, const char *pb);

bool compare_prefix_eq(const char *e1, const char *e2);

unsigned long long file_size(const char *filename);

uint64_t readcontent_unlocked1(FILE *handle, char *p, uint64_t isize);

bool compare_dp_char_eq(const char *pa, const char *pb);

inline int cmp64val(int64_t ia, int64_t ib)
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

inline int64_t to64le8(const char *s, int len)
{
    int64_t ret = 0;
    int i = 56;
    while (1)
    {
        ret |= ((int64_t)(*s) << i);
        --s;
        --len;
        if (len == 0)
        {
            return ret;
        }
        i -= 8;
    }
    return ret;
}

inline int64_t to64le8h(const char *s, int len)
{
    int64_t ret = 0;
    int i = 56;
    while (1)
    {
        ret |= ((int64_t)(*s) << i);
        ++s;
        --len;
        if (len == 0)
        {
            return ret;
        }
        i -= 8;
    }
    return ret;
}

inline int cmpbuf_dp(const char *pa, int na, const char *pb, int nb)
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
    if (na > 0 && nb > 0)
    {
        int nc = min(na, nb);
        int64_t ia = to64le8(pa + na - 1, nc);
        int64_t ib = to64le8(pb + nb - 1, nc);
        return cmp64val(ia, ib);
    }
    return 0;
}

inline int cmpbuf_pf(const char *pa, int na, const char *pb, int nb)
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
    if (na >= 8 && nb > 0)
    {
        int64_t ia = (*(int64_t *)pa) & temp[nb];
        int64_t ib = to64le8h(pb, nb);
        // printf("ia=%08x,na=%d,ib=%08x,nb=%d\n", ia, na, ib, nb);
        return cmp64val(ia, ib);
    }
    if (nb >= 8 && na > 0)
    {
        int64_t ia = to64le8h(pa, na);
        int64_t ib = (*(int64_t *)pb) & temp[na];
        // printf("ia=%08x,na=%d,ib=%08x,nb=%d\n", ia, na, ib, nb);
        return cmp64val(ia, ib);
    }
    if (na > 0 && nb > 0)
    {
        int nc = min(na, nb);
        int64_t ia = to64le8h(pa, nc);
        int64_t ib = to64le8h(pb, nc);
        return cmp64val(ia, ib);
    }
    return 0;
}

#endif