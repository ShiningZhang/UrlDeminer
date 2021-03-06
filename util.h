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
#include <assert.h>

using namespace std;

// #define FILESPLITSIZE 23000000
// #define SMALLSIZE 110000
#define FILESPLITSIZE 64 * 1024 * 1024
#define SMALLSIZE 1100000000

#define SMALLFILESIZE 10 * 1024 * 1024
#define MAXSORT 8
#define VERYSMALLSIZE 1000000

#define BUFHEADSIZE 10

#define INITURLCOUNT 6000000

#define DOMAIN_CHAR_COUNT 41 //- . / 0-9 : a-z _
// #define DOMAIN_CHAR_COUNT 38 //26 + 10 + 2
// #define DOMAIN_CHAR_COUNT2 40 //26 + 10 + 2

#define MAX_MEM_SIZE 12000000000

#define MAX_USE_MEM_SIZE 7000000000

#define MEM_SIZE_256 64 * 1024 * 1024

#define INT32_WWW ((int32_t)(0x2e77777777))

#ifdef DEBUG
#define LOG(format_string, ...)                                                                \
    {                                                                                          \
        struct timeval __val;                                                                  \
        gettimeofday(&__val, NULL);                                                            \
        printf("%ld.%03ld " format_string, __val.tv_sec, __val.tv_usec / 1000, ##__VA_ARGS__); \
    }
#else
#define LOG(format_string, ...)
#endif

class DomainFilter;
class PrefixFilter;
class UrlFilter;
class UrlPFFilter;
class Request;
class SPFFilter;

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
    FILE *fp_[(DOMAIN_CHAR_COUNT+1)];
    size_t size_[(DOMAIN_CHAR_COUNT+1)];
    int idx;
    size_t total_size_;
    int count_[(DOMAIN_CHAR_COUNT+1)];
};

struct FileElementLarge : public FileElement
{
    int count1_[(DOMAIN_CHAR_COUNT+1)][DOMAIN_CHAR_COUNT];
    size_t size1_[(DOMAIN_CHAR_COUNT+1)][DOMAIN_CHAR_COUNT];
};

struct FileElementPrefix
{
    FILE *fp_;
    size_t wt_size_;
    size_t rd_size_;
    size_t size1_[(DOMAIN_CHAR_COUNT+1)];
    size_t size2_[(DOMAIN_CHAR_COUNT+1)][DOMAIN_CHAR_COUNT];
    int idx_;
    int count_[(DOMAIN_CHAR_COUNT+1)][DOMAIN_CHAR_COUNT][3][2];
};

struct stPFCMPOFFSET
{
    int64_t *pa_;
    int offset_;
    uint16_t na_;
    int64_t num_;
};

extern uint64_t temp[9];
extern int domain_temp[256];
extern int domain_temp2[256];

extern queue<UrlFilter *> gQueue;
extern queue<UrlFilter *> gQueueCache;
extern mutex gMutex;
extern condition_variable gCV;
extern bool gStart;
extern queue<UrlPFFilter *> gQueueFilter;
extern queue<SPFFilter *> gQUrlPfTask;
extern mutex gMutexUrlPfTask;
extern condition_variable gCVUrlPfTask;

extern Request *gRequest;
extern mutex gMCount;

extern bool gEnd;
extern bool dp_need[DOMAIN_CHAR_COUNT];
extern bool dp_sp_need[2][DOMAIN_CHAR_COUNT];
extern bool pf_need[DOMAIN_CHAR_COUNT][DOMAIN_CHAR_COUNT];

size_t readcontent_unlocked(FILE *handle, char *p, uint64_t isize, int *end);
uint64_t sizeoffile(FILE *handle);
int setfilesplitsize(uint64_t inputfilesize, int maxsort);

// inline int cmp64val(int64_t ia, int64_t ib);
// inline int cmpbuf_dp(const char *pa, int na, const char *pb, int nb);
bool compare_dp(const DomainPortBuf &e1, const DomainPortBuf &e2);
int compare_dp_eq(const DomainPortBuf *e1, const DomainPortBuf *e2);
// inline int cmpbuf_pf(const char *pa, int na, const char *pb, int nb);
bool compare_prefix(const char *e1, const char *e2);
bool compare_prefix_1(const char *e1, const char *e2);

void arrangesuffix(char *s, int len);

bool compare_dp_char(const char *pa, const char *pb);

bool compare_prefix_eq(const char *e1, const char *e2);

unsigned long long file_size(const char *filename);

uint64_t readcontent_unlocked1(FILE *handle, char *p, uint64_t isize);

uint64_t readcontent(FILE *handle, char *&p, uint64_t isize);

bool compare_dp_char_eq(const char *pa, const char *pb);

bool cmp_pf_loop1(const stPFCMPOFFSET &e1, const char *e2);

char **unique_pf(char **first, char **last);

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

inline uint16_t eqlen64(int64_t r)
{
    uint16_t c = 7;
    if ((r & temp[1]) != 0)
        return (uint16_t)0;
    while (c > 0)
    {
        r >>= 8;
        if (r == 0)
            break;
        --c;
    }
    return c;
}
inline uint16_t pf_eq_len(const char *pa, int na, const char *pb, int nb)
{
    int64_t ret = 0;
    uint16_t count = 0;
    while (na >= 8 && nb >= 8)
    {
        int64_t ia = *(int64_t *)pa;
        int64_t ib = *(int64_t *)pb;
        ret = ia - ib;
        if (ret != 0)
        {
            return count + eqlen64(ret);
        }
        na -= 8;
        nb -= 8;
        pa += 8;
        pb += 8;
        count += 8;
    }
    if (na >= 8 && nb > 0)
    {
        int64_t ia = (*(int64_t *)pa) & temp[nb];
        int64_t ib = to64le8h(pb, nb);
        // printf("ia=%08x,na=%d,ib=%08x,nb=%d\n", ia, na, ib, nb);
        ret = ia - ib;
        if (ret == 0)
            return count + nb;
        else
            return count + eqlen64(ret);
    }
    else if (nb >= 8 && na > 0)
    {
        int64_t ia = to64le8h(pa, na);
        int64_t ib = (*(int64_t *)pb) & temp[na];
        // printf("ia=%08x,na=%d,ib=%08x,nb=%d\n", ia, na, ib, nb);
        ret = ia - ib;
        if (ret == 0)
            return count + na;
        else
            return count + eqlen64(ret);
    }
    else if (na > 0 && nb > 0)
    {
        int nc = min(na, nb);
        int64_t ia = to64le8h(pa, nc);
        int64_t ib = to64le8h(pb, nc);
        ret = ia - ib;
        if (ret == 0)
            return count + nc;
        else
            return count + eqlen64(ret);
    }

    return count;
}

#endif