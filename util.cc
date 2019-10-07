#include "util.h"

uint64_t sizeoffile(FILE *handle)
{
	uint64_t filesize;
	long oldpos = ftell(handle);
	fseek(handle, 0, SEEK_END);
	filesize = ftell(handle);
	fseek(handle, oldpos, SEEK_SET);
	return filesize;
}

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

int cmp64val(int64_t ia, int64_t ib)
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

int cmpbuf_dp(const char *pa, int na, const char *pb, int nb)
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

bool compare_dp(const DomainPortBuf &e1, const DomainPortBuf &e2)
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
    if (e1.hit < e2.hit)
        return true;
    else if (e1.hit > e2.hit)
        return false;
    return pa < pb;
}

int cmpbuf_pf(const char *pa, int na, const char *pb, int nb)
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
    int ret = cmpbuf_pf(pa, na, pb, nb);
    if (ret != 0)
        return ret == -1 ? true : false;
    if (na < nb)
        return true;
    else if (na > nb)
        return false;
    return e1<e2;
}