#include "util.h"

#include <sys/stat.h>

queue<UrlFilter *> gQueueCache;
queue<UrlFilter *> gQueue;
mutex gMutex;
condition_variable gCV;

FilterCounters::FilterCounters()
{
    memset(this, 0, sizeof(*this));
}

uint64_t sizeoffile(FILE *handle)
{
    uint64_t filesize;
    long oldpos = ftell(handle);
    fseek(handle, 0, SEEK_END);
    filesize = ftell(handle);
    fseek(handle, oldpos, SEEK_SET);
    return filesize;
}

unsigned long long file_size(const char *filename)
{
    unsigned long long size;
    struct stat st;

    stat(filename, &st);
    size = st.st_size;

    return size;

} //get_file_size

size_t readcontent_unlocked(FILE *handle, char *p, uint64_t isize, int *end)
{
    long oldpos = ftell(handle);
    uint64_t size = fread_unlocked(p, 1, isize, handle);
    if (size < isize)
    {
        *end = 1;
    }
    else
    {
        *end = 0;
    }
    while (size > 0 && p[size - 1] != '\n')
    {
        --size;
    }
    fseek(handle, oldpos + size, SEEK_SET);
    return size;
}

uint64_t readcontent_unlocked1(FILE *handle, char *p, uint64_t isize)
{
    uint64_t size = fread_unlocked(p, 1, isize, handle);
    uint64_t offset = size;
    while (offset > 0 && p[offset - 1] != '\n')
    {
        --offset;
    }
    fseek(handle, -(size - offset), SEEK_CUR);
    return offset;
}

int setfilesplitsize(uint64_t inputfilesize, int maxsort)
{
    uint64_t filesplitsize;
    filesplitsize = inputfilesize / maxsort + 129;
    if (filesplitsize > FILESPLITSIZE)
    {
        filesplitsize = FILESPLITSIZE;
    }
    return (uint32_t)filesplitsize;
}

void arrangesuffix(char *s, int len)
{
    while (len >= 8)
    {
        char t;
        t = s[0];
        s[0] = s[7];
        s[7] = t;
        t = s[1];
        s[1] = s[6];
        s[6] = t;
        t = s[2];
        s[2] = s[5];
        s[5] = t;
        t = s[3];
        s[3] = s[4];
        s[4] = t;
        s += 8;
        len -= 8;
    }
}

/* inline int cmp64val(int64_t ia, int64_t ib)
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
} */

/* static inline int64_t to64le8(const char *s, int len)
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
} */

/* static inline int64_t to64le8h(const char *s, int len)
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
} */

/* inline int cmpbuf_dp(const char *pa, int na, const char *pb, int nb)
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
} */

bool compare_dp(const DomainPortBuf &e1, const DomainPortBuf &e2)
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
    if (ret != 0)
        return ret == -1 ? true : false;
    if (na < nb)
        return true;
    else if (na > nb)
        return false;
    if (e1.hit < e2.hit)
        return true;
    else if (e1.hit > e2.hit)
        return false;
    return pa < pb;
}

bool compare_dp_char(const char *pa, const char *pb)
{
    uint16_t na = *(uint16_t *)(pa - 2);
    uint16_t nb = *(uint16_t *)(pb - 2);
    int ret = cmpbuf_dp(pa, na, pb, nb);
    if (ret != 0)
        return ret == -1 ? true : false;
    int sub = na - nb;
    if (sub < 0)
        return true;
    else if (sub > 0)
        return false;
    else
    {
        return pa < pb;
    }
}

bool compare_dp_char_eq(const char *pa, const char *pb)
{
    uint16_t na = *(uint16_t *)(pa - 2);
    uint16_t nb = *(uint16_t *)(pb - 2);
    int ret = cmpbuf_dp(pa, na, pb, nb);
    return ret;
}

uint64_t temp[9] = {
    0x0000000000000000,
    0xff00000000000000,
    0xffff000000000000,
    0xffffff0000000000,
    0xffffffff00000000,
    0xffffffffff000000,
    0xffffffffffff0000,
    0xffffffffffffff00,
    0xffffffffffffffff};

/* int inline cmpbuf_pf(const char *pa, int na, const char *pb, int nb)
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
} */

bool compare_prefix(const char *e1, const char *e2)
{
    const char *pa = e1;
    const char *pb = e2;
    int na = (int)*((uint16_t *)pa);
    int nb = (int)*((uint16_t *)pb);
    pa += 2;
    pb += 2;
    int ret = cmpbuf_pf(pa, na, pb, nb);
    if (ret != 0)
        return ret == -1 ? true : false;
    if (na < nb)
        return true;
    else if (na > nb)
        return false;
    return e1 < e2;
}

bool compare_prefix_eq(const char *e1, const char *e2)
{
    const char *pa = e1;
    const char *pb = e2;
    int na = (int)*((uint16_t *)pa);
    int nb = (int)*((uint16_t *)pb);
    pa += 2;
    pb += 2;
    int ret = cmpbuf_pf(pa, na, pb, nb);
    return ret;
}