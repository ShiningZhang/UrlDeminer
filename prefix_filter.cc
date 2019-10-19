#include "prefix_filter.h"

#include <string.h>
#include <string>

#include "pdqsort.h"

#include "util.h"

using namespace std;

PrefixFilter::PrefixFilter()
{
    p_ = NULL;
    buf_size_ = 0;
    memset(size_, 0, 3 * 2 * 2 * sizeof(uint64_t) * DOMAIN_CHAR_COUNT);
    memset(list_range_, 0, 3 * 2 * 2 * sizeof(int *) * DOMAIN_CHAR_COUNT);
    memset(list_count_, 0, 3 * 2 * 2 * sizeof(int) * DOMAIN_CHAR_COUNT);
}

PrefixFilter::~PrefixFilter()
{
    if (p_ != NULL)
    {
        p_ = p_ - BUFHEADSIZE;
        free(p_);
        p_ = NULL;
    }
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < 2; ++k)
            {
                for (int m = 0; m < DOMAIN_CHAR_COUNT; ++m)
                {
                    if (list_count_[i][j][k][m] > 0)
                    {
                        free(list_https_[i][j][k][m]);
                        list_https_[i][j][k][m] = NULL;
                    }
                    if (list_range_[i][j][k][m] != NULL)
                    {
                        free(list_range_[i][j][k][m]);
                        list_range_[i][j][k][m] = NULL;
                    }
                }
            }
        }
    }
    for (uint i = 0; i < list_str_.size(); ++i)
    {
        free(list_str_[i]);
    }
}

inline int find_port(char *s)
{
    char *domainend = strchr(s, '/');
    char *offset = domainend - 1;
    char *start = s > domainend - 6 ? s : domainend - 6;
    while (offset > start)
    {
        if (*offset != ':')
            --offset;
        else
            break;
    }
    if (*offset == ':')
    {
        return atoi(offset + 1);
    }
    return 0;
}

void PrefixFilter::prepare_buf(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        uint8_t type = se[-3] == '=' ? 2 : se[-3] == '+' ? 1 : 0;
        bool hit = se[-1] == '-' ? true : false;
        if (s[0] == '/')
        {
            int t = domain_temp[((unsigned char)*(s + 2))];
            ++this->list_count_[type][hit][0][t];
            ++this->list_count_[type][hit][1][t];
        }
        else if (s[7] == '/')
        {
            int t = domain_temp[((unsigned char)*(s + 8))];
            ++this->list_count_[type][hit][1][t];
        }
        else
        {
            int t = domain_temp[((unsigned char)*(s + 7))];
            ++this->list_count_[type][hit][0][t];
        }
        s = se + 1;
    }
}

/* void PrefixFilter::load_(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        int port_type = 0;
        if (s[0] == '/')
        {
            s += 2;
            port_type = 0; //80 | 443
        }
        else if (s[7] == '/')
        {
            s += 8;
            port_type = 2; // 443
        }
        else
        {
            s += 7;
            port_type = 1; // 80
        }
        char *domainend = strchr(s, '/');
        char *offset = domainend;
        while (offset > s && offset > (domainend - 6))
        {
            if (*offset != ':')
                --offset;
            else
                break;
        }

        if (*offset == ':')
        {
            char *offset_1 = offset + 1;
            int port_size = domainend - offset_1;
            if (port_type == 1 && port_size == 2 && memcmp(offset_1, "80", 2) == 0)
            {
                memmove(s + 3, s, offset - s - 1);
                s += 3;
            }
            else if (port_type == 2 && port_size == 3 && memcmp(offset_1, "443", 3) == 0)
            {
                memmove(s + 4, s, offset - s);
                s += 4;
            }
            else if (port_type == 0)
            {
                if (port_size == 2 && memcmp(offset_1, "80", 2) == 0)
                {
                    memmove(s + 3, s, offset - s);
                    s += 3;
                    port_type = 4;
                }
                else if (port_size == 3 && memcmp(offset_1, "443", 3) == 0)
                {
                    port_type = 5;
                }
            }
        }
        uint16_t len = se - s - 4;
        *(uint16_t *)(s - 2) = len;
        uint8_t type = se[-3] == '=' ? 2 : se[-3] == '+' ? 1 : 0;
        bool hit = se[-1] == '-' ? true : false;
        if (port_type > 2)
        {
            s[-3] = 1;
        }
        else
        {
            s[-3] = 0xff & port_type;
        }
        if (port_type == 1)
        {
            list_https_[type][hit][0].push_back(s - 2);
            size_[type][hit][0] += len + 3;
        }
        else if (port_type == 2)
        {
            list_https_[type][hit][1].push_back(s - 2);
            size_[type][hit][1] += len + 3;
        }
        else if (port_type == 0)
        {
            list_https_[type][hit][0].push_back(s - 2);
            size_[type][hit][0] += len + 3;
            list_https_[type][hit][1].push_back(s - 2);
            size_[type][hit][1] += len + 3;
        }
        else if (port_type == 4)
        {
            list_https_[type][hit][0].push_back(s - 2);
            size_[type][hit][0] += len + 3;
            int str_length = 2 + (offset - s) + 3 + (se - domainend - 4) + 1;
            char *str = (char *)malloc(str_length);
            int tmp = 0;
            *((uint16_t *)(str)) = 0xffff & (len + 6);
            tmp += 2;
            memcpy(str + tmp, s, offset - s);
            tmp += offset - s;
            memcpy(str + tmp, ":80", 3);
            tmp += 3;
            memcpy(str + tmp, domainend, se - domainend - 4);
            tmp += se - domainend - 4;
            *(str + tmp) = 0xff & 2;
            ++tmp;
            arrangesuffix(str + 2, len + 3);
            list_str_.emplace_back(str);
            list_https_[type][hit][1].push_back(str);
            size_[type][hit][1] += len + 6;
        }
        else if (port_type == 5)
        {
            list_https_[type][hit][0].push_back(s - 2);
            size_[type][hit][0] += len + 3;
            int str_length = 2 + (offset - s) + (se - domainend - 4) + 1;
            char *str = (char *)malloc(str_length);
            int tmp = 0;
            *((uint16_t *)(str)) = 0xffff & (len - 1);
            tmp += 2;
            memcpy(str + tmp, s, offset - s);
            tmp += offset - s;
            memcpy(str + tmp, domainend, se - domainend - 4);
            tmp += se - domainend - 4;
            *(str + tmp) = 0xff & 2;
            arrangesuffix(str + 2, len - 4);
            list_str_.emplace_back(str);
            list_https_[type][hit][1].push_back(str);
            size_[type][hit][1] += len - 1;
        }
        arrangesuffix(s, len);
#ifdef DEBUG
        *se = '\0';
        printf("PrefixFilter::load_:%d,%s\n", len, s);
#endif
        s = se + 1;
    }
} */

void PrefixFilter::load_(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    int count[3][2][2][DOMAIN_CHAR_COUNT] = {0};
    while (s < e)
    {
        char *se = strchr(s, '\n');
        int port_type = 0;
        if (s[0] == '/')
        {
            s += 2;
            port_type = 0; //80 | 443
        }
        else if (s[7] == '/')
        {
            s += 8;
            port_type = 2; // 443
        }
        else
        {
            s += 7;
            port_type = 1; // 80
        }
        int t = domain_temp[((unsigned char)(*s))];
        char *domainend = strchr(s, '/');
        char *offset = domainend;
        while (offset > s && offset > (domainend - 6))
        {
            if (*offset != ':')
                --offset;
            else
                break;
        }
        if (*offset == ':')
        {
            char *offset_1 = offset + 1;
            int port_size = domainend - offset_1;
            if (port_type == 1 && port_size == 2 && memcmp(offset_1, "80", 2) == 0)
            {
                memmove(s + 3, s, offset - s - 1);
                s += 3;
            }
            else if (port_type == 2 && port_size == 3 && memcmp(offset_1, "443", 3) == 0)
            {
                memmove(s + 4, s, offset - s);
                s += 4;
            }
            else if (port_type == 0)
            {
                if (port_size == 2 && memcmp(offset_1, "80", 2) == 0)
                {
                    memmove(s + 3, s, offset - s);
                    s += 3;
                    port_type = 4;
                }
                else if (port_size == 3 && memcmp(offset_1, "443", 3) == 0)
                {
                    port_type = 5;
                }
            }
        }
        uint16_t len = se - s - 4;
        *(uint16_t *)(s - 2) = len - 1;
        uint8_t type = se[-3] == '=' ? 2 : se[-3] == '+' ? 1 : 0;
        bool hit = se[-1] == '-' ? true : false;
        if (port_type > 2)
        {
            s[-3] = 1;
        }
        else
        {
            s[-3] = 0xff & port_type;
        }
        if (port_type == 1)
        {
            list_https_[type][hit][0][t][count[type][hit][0][t]] = (s - 2);
            size_[type][hit][0][t] += len + 3;
            ++count[type][hit][0][t];
        }
        else if (port_type == 2)
        {
            list_https_[type][hit][1][t][count[type][hit][1][t]] = (s - 2);
            size_[type][hit][1][t] += len + 3;
            ++count[type][hit][1][t];
        }
        else if (port_type == 0)
        {
            list_https_[type][hit][0][t][count[type][hit][0][t]] = (s - 2);
            size_[type][hit][0][t] += len + 3;
            ++count[type][hit][0][t];
            list_https_[type][hit][1][t][count[type][hit][1][t]] = (s - 2);
            size_[type][hit][1][t] += len + 3;
            ++count[type][hit][1][t];
        }
        else if (port_type == 4)
        {
            list_https_[type][hit][0][t][count[type][hit][0][t]] = (s - 2);
            size_[type][hit][0][t] += len + 3;
            ++count[type][hit][0][t];
            int str_length = 2 + (offset - s) + 3 + (se - domainend - 4) + 1;
            char *str = (char *)malloc(str_length);
            int tmp = 0;
            *((uint16_t *)(str)) = 0xffff & (len + 6);
            tmp += 2;
            memcpy(str + tmp, s, offset - s);
            tmp += offset - s;
            memcpy(str + tmp, ":80", 3);
            tmp += 3;
            memcpy(str + tmp, domainend, se - domainend - 4);
            tmp += se - domainend - 4;
            *(str + tmp) = 0xff & 2;
            ++tmp;
            arrangesuffix(str + 2, len + 3);
            list_str_.emplace_back(str);
            list_https_[type][hit][1][t][count[type][hit][1][t]] = str;
            size_[type][hit][1][t] += len + 6;
            ++count[type][hit][1][t];
        }
        else if (port_type == 5)
        {
            list_https_[type][hit][0][t][count[type][hit][0][t]] = (s - 2);
            ++count[type][hit][0][t];
            size_[type][hit][0][t] += len + 3;
            int str_length = 2 + (offset - s) + (se - domainend - 4) + 1;
            char *str = (char *)malloc(str_length);
            int tmp = 0;
            *((uint16_t *)(str)) = 0xffff & (len - 1);
            tmp += 2;
            memcpy(str + tmp, s, offset - s);
            tmp += offset - s;
            memcpy(str + tmp, domainend, se - domainend - 4);
            tmp += se - domainend - 4;
            *(str + tmp) = 0xff & 2;
            arrangesuffix(str + 2, len - 4);
            list_str_.emplace_back(str);
            list_https_[type][hit][1][t][count[type][hit][1][t]] = str;
            ++count[type][hit][1][t];
            size_[type][hit][1][t] += len - 1;
        }
        arrangesuffix(s + 1, len - 1);
#ifdef DEBUG
        *se = '\0';
        printf("PrefixFilter::load_:%d,%s\n", len, s);
#endif
        s = se + 1;
    }
}

PrefixFilter *PrefixFilter::load(char *p, uint64_t size)
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
    PrefixFilter *filter = new PrefixFilter();
    filter->prepare_buf(p, size);
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < 2; ++k)
            {
                for (int m = 0; m < DOMAIN_CHAR_COUNT; ++m)
                {
                    if (filter->list_count_[i][j][k][m] > 0)
                    {
                        filter->list_https_[i][j][k][m] = (char **)malloc(filter->list_count_[i][j][k][m] * sizeof(char *));
                    }
                }
            }
        }
    }
    filter->load_(p, size);
    filter->p_ = p;
    filter->buf_size_ = size;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < 2; ++k)
            {
                for (int m = 0; m < DOMAIN_CHAR_COUNT; ++m)
                {
                    if (filter->list_count_[i][j][k][m] > 0)
                    {
                        pdqsort(filter->list_https_[i][j][k][m], filter->list_https_[i][j][k][m] + filter->list_count_[i][j][k][m], compare_prefix);
                    }
                }
            }
        }
    }
    /*for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < 2; ++k)
            {
                filter->prepare_range(filter->list_https_[i][j][k], filter->list_count_[i][j][k], filter->list_range_[i][j][k]);
            }
        }
    } */
    return filter;
}

void PrefixFilter::prepare_range(char **list, int size, int *&range)
{
    if (size == 0)
        return;
    range = (int *)malloc(size * sizeof(int));
    char *pa = list[0];
    range[0] = 1;
    for (int i = 1; i < size; ++i)
    {
        char *pb = list[i];
        if (compare_prefix_eq(pa, pb) != 0)
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

/* PrefixFilter *PrefixFilter::merge(vector<PrefixFilter *> prefix_filter_list)
{
    PrefixFilter *filter = new PrefixFilter();
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < 2; ++k)
            {
                int size = 0;
                for (int m = 0; m < prefix_filter_list.size(); ++m)
                {
                    size += prefix_filter_list[m]->list_count_[i][j][k];
                }
                if (size > 0)
                {
                    filter->list_count_[i][j][k] = size;
                    filter->list_https_[i][j][k] = (char **)malloc(filter->list_count_[i][j][k] * sizeof(char *));
                    size = 0;
                    for (int m = 0; m < prefix_filter_list.size(); ++m)
                    {
                        memcpy(filter->list_https_[i][j][k] + size, prefix_filter_list[m]->list_https_[i][j][k], prefix_filter_list[m]->list_count_[i][j][k] * sizeof(char *));
                        size += prefix_filter_list[m]->list_count_[i][j][k];
                    }
                    pdqsort(filter->list_https_[i][j][k], filter->list_https_[i][j][k] + filter->list_count_[i][j][k], compare_prefix);
                    filter->prepare_range(filter->list_https_[i][j][k], filter->list_count_[i][j][k], filter->list_range_[i][j][k]);
#ifdef DEBUG
                    for (int n = 0; n < filter->list_count_[i][j][k]; ++n)
                    {
                        printf("http[%d,%d,%d]:%s\n", i, j, k, *(filter->list_https_[i][j][k] + n) + 2);
                    }
#endif
                }
            }
        }
    }
    return filter;
} */

#ifdef VER1
int PrefixFilterMerge::merge(vector<PrefixFilter *> list, int idx)
{
    int size = 0;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < 2; ++k)
            {
                size = 0;
                for (uint m = 0; m < list.size(); ++m)
                {
                    size += list[m]->list_count_[i][j][k][idx];
                }
                if (size > 0)
                {
                    list_count_[i][j][k][idx] = size;
                    list_https_[i][j][k][idx] = (char **)malloc(size * sizeof(char *));
                    size = 0;
                    for (uint m = 0; m < list.size(); ++m)
                    {
                        if (list[m]->list_count_[i][j][k][idx] > 0)
                        {
                            memcpy(list_https_[i][j][k][idx] + size, list[m]->list_https_[i][j][k][idx], list[m]->list_count_[i][j][k][idx] * sizeof(char *));
                            size += list[m]->list_count_[i][j][k][idx];
                        }
                    }
                    pdqsort(list_https_[i][j][k][idx], list_https_[i][j][k][idx] + list_count_[i][j][k][idx], compare_prefix);
                    prepare_range(list_https_[i][j][k][idx], list_count_[i][j][k][idx], list_range_[i][j][k][idx]);
                }
            }
        }
    }
    return 0;
}
#endif

struct HeapItemPf
{
    int ed_;
    int idx_;
    int start_;
    char **list_;
    inline bool operator<(const HeapItemPf &y) const
    {
        if (idx_ == 0 || y.idx_ == 0)
            return idx_ < y.idx_;
        return compare_prefix(list_[start_], y.list_[y.start_]);
    }
};

typedef HeapItemPf *pHeapItemPf;

HeapItemPf MinHIpf = {0, 0, 0, 0};

static inline void LoserAdjust(int *ls, const HeapItemPf *arr, int s, const int n)
{
    int t1 = ((s + n) >> 1);
    s = ls[s + n];
    while (t1 > 0)
    {
        if (arr[ls[t1]] < arr[s])
        {
            s ^= ls[t1];
            ls[t1] ^= s;
            s ^= ls[t1];
        }
        t1 >>= 1;
    }
    ls[0] = s;
}

static inline void LoserBuild(int *ls, const HeapItemPf *arr, const int n)
{
    for (int i1 = 0; i1 < n; i1++)
    {
        ls[i1] = n;
        ls[i1 + n] = i1;
    }
    for (int i1 = n - 1; i1 >= 0; i1--)
    {
        LoserAdjust(ls, arr, i1, n);
    }
}

int PrefixFilterMerge::merge(vector<PrefixFilter *> list, int idx)
{
    int size = 0;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < 2; ++k)
            {
                size = 0;
                for (uint m = 0; m < list.size(); ++m)
                {
                    size += list[m]->list_count_[i][j][k][idx];
                }
                if (size > 0)
                {
                    list_https_[i][j][k][idx] = (char **)malloc(size * sizeof(char *));
                    list_count_[i][j][k][idx] = size;
                    size = 0;
                    {
                        size_t part_num = list.size();
                        int *loser = (int *)malloc((2 * part_num + 1) * sizeof(int));
                        int sl = 0;
                        HeapItemPf *heap = (HeapItemPf *)malloc((part_num + 1) * sizeof(HeapItemPf));
                        memset(heap, 0, (part_num + 1) * sizeof(HeapItemPf));
                        for (int m = 0; m < part_num; ++m)
                        {
                            if (list[m]->list_count_[i][j][k][idx] > 0)
                            {
                                // HeapItemDp *pHI = heap + sl++;
                                heap[sl].ed_ = list[m]->list_count_[i][j][k][idx] - 1;
                                heap[sl].start_ = 0;
                                heap[sl].list_ = list[m]->list_https_[i][j][k][idx];
                                heap[sl].idx_ = part_num;
                                // SP_DEBUG("k=%d,sl=%d,ed=%d,list=%p\n", k, sl, heap[sl].ed_, heap[sl].list_);
                                ++sl;
                            }
                        }
                        if (sl > 0)
                        {
                            int wlen = 0;
                            if (sl > 1)
                            {
                                heap[sl] = MinHIpf;
                                LoserBuild(loser, heap, sl);
                                while (sl > 1)
                                {
                                    int win = loser[0];
                                    pHeapItemPf pHI = heap + win;
                                    list_https_[i][j][k][idx][size++] = pHI->list_[pHI->start_];
                                    if (pHI->start_ < pHI->ed_)
                                    {
                                        ++pHI->start_;
                                        LoserAdjust(loser, heap, win, sl);
                                    }
                                    else
                                    {
                                        if (win < --sl)
                                        {
                                            heap[win] = heap[sl];
                                        }
                                        if (sl > 1)
                                        {
                                            heap[sl] = MinHIpf;
                                            LoserBuild(loser, heap, sl);
                                        }
                                    }
                                }
                            }
                            if (sl == 1)
                            {
                                pHeapItemPf pHI = heap + 0;
                                memcpy(list_https_[i][j][k][idx] + size, pHI->list_ + pHI->start_, (pHI->ed_ - pHI->start_ + 1) * sizeof(char *));
                            }
                        }
                        free(loser);
                        free(heap);
                    }
                    prepare_range(list_https_[i][j][k][idx], list_count_[i][j][k][idx], list_range_[i][j][k][idx]);
                }
            }
        }
    }
    return 0;
}

/* int PrefixFilterMerge::merge_large(vector<PrefixFilter *> list, int idx)
{
    int size = 0;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < 2; ++k)
            {
                for (int n = 0; n < DOMAIN_CHAR_COUNT; ++n)
                {
                    size = 0;
                    for (uint m = 0; m < list.size(); ++m)
                    {
                        size += list[m]->list_count_large_[i][j][k][idx][n];
                    }
                    if (size > 0)
                    {
                        list_https_large_[i][j][k][idx][n] = (char **)malloc(size * sizeof(char *));
                        list_count_large_[i][j][k][idx][n] = size;
                        size = 0;
                        {
                            size_t part_num = list.size();
                            int *loser = (int *)malloc((2 * part_num + 1) * sizeof(int));
                            int sl = 0;
                            HeapItemPf *heap = (HeapItemPf *)malloc((part_num + 1) * sizeof(HeapItemPf));
                            memset(heap, 0, (part_num + 1) * sizeof(HeapItemPf));
                            for (int m = 0; m < part_num; ++m)
                            {
                                if (list[m]->list_count_large_[i][j][k][idx][n] > 0)
                                {
                                    // HeapItemDp *pHI = heap + sl++;
                                    heap[sl].ed_ = list[m]->list_count_large_[i][j][k][idx][n] - 1;
                                    heap[sl].start_ = 0;
                                    heap[sl].list_ = list[m]->list_https_large_[i][j][k][idx][n];
                                    heap[sl].idx_ = part_num;
                                    // SP_DEBUG("k=%d,sl=%d,ed=%d,list=%p\n", k, sl, heap[sl].ed_, heap[sl].list_);
                                    ++sl;
                                }
                            }
                            if (sl > 0)
                            {
                                int wlen = 0;
                                if (sl > 1)
                                {
                                    heap[sl] = MinHIpf;
                                    LoserBuild(loser, heap, sl);
                                    while (sl > 1)
                                    {
                                        int win = loser[0];
                                        pHeapItemPf pHI = heap + win;
                                        list_https_large_[i][j][k][idx][n][size++] = pHI->list_[pHI->start_];
                                        if (pHI->start_ < pHI->ed_)
                                        {
                                            ++pHI->start_;
                                            LoserAdjust(loser, heap, win, sl);
                                        }
                                        else
                                        {
                                            if (win < --sl)
                                            {
                                                heap[win] = heap[sl];
                                            }
                                            if (sl > 1)
                                            {
                                                heap[sl] = MinHIpf;
                                                LoserBuild(loser, heap, sl);
                                            }
                                        }
                                    }
                                }
                                if (sl == 1)
                                {
                                    pHeapItemPf pHI = heap + 0;
                                    memcpy(list_https_large_[i][j][k][idx][n] + size, pHI->list_ + pHI->start_, (pHI->ed_ - pHI->start_ + 1) * sizeof(char *));
                                }
                            }
                            free(loser);
                            free(heap);
                        }
                        // prepare_range(list_https_[i][j][k][idx], list_count_[i][j][k][idx], list_range_[i][j][k][idx]);
                    }
                }
            }
        }
    }
    return 0;
} */

void PrefixFilterMerge::cpy_filter_list(vector<PrefixFilter *> &list)
{
    for (uint i = 0; i < list.size(); ++i)
    {
        p_list_.push_back(list[i]->p_ - BUFHEADSIZE);
        buf_size_list_.push_back(list[i]->buf_size_);
        list[i]->p_ = NULL;
        list[i]->buf_size_ = 0;
        delete list[i];
    }
    list.clear();
}

PrefixFilterMerge::PrefixFilterMerge()
{
}

PrefixFilterMerge::~PrefixFilterMerge()
{
    for (size_t i = 0; i < p_list_.size(); ++i)
    {
        if (p_list_[i] != NULL)
        {
            free(p_list_[i]);
        }
    }
    p_list_.clear();
    buf_size_list_.clear();
}

PrefixFilterLargeLoad::PrefixFilterLargeLoad()
{
    p_ = NULL;
    buf_size_ = 0;
    memset(list_count_large_, 0, 3 * 2 * sizeof(int) * DOMAIN_CHAR_COUNT * DOMAIN_CHAR_COUNT);
    memset(list_cc_, 0, 3 * 2 * 2 * sizeof(int) * DOMAIN_CHAR_COUNT * DOMAIN_CHAR_COUNT);
}

PrefixFilterLargeLoad::~PrefixFilterLargeLoad()
{
    if (p_ != NULL)
    {
        p_ = p_ - BUFHEADSIZE;
        free(p_);
        p_ = NULL;
    }
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            // for (int k = 0; k < 2; ++k)
            {
                for (int m = 0; m < DOMAIN_CHAR_COUNT; ++m)
                {
                    for (int n = 0; n < DOMAIN_CHAR_COUNT; ++n)
                    {
                        if (list_count_large_[i][j][m][n] > 0)
                        {
                            list_https_large_[i][j][m][n] = (char **)malloc(list_count_large_[i][j][m][n] * sizeof(char *));
                        }
                    }
                }
            }
        }
    }
}

void PrefixFilterLargeLoad::prepare_buf_large(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        uint8_t type = se[-3] == '=' ? 2 : se[-3] == '+' ? 1 : 0;
        bool hit = se[-1] == '-' ? true : false;
        if (s[0] == '/')
        {
            int t = domain_temp[((unsigned char)*(s + 2))];
            int t1 = domain_temp[((unsigned char)*(s + 3))];
            ++this->list_count_large_[type][hit][t][t1];
        }
        else if (s[7] == '/')
        {
            int t = domain_temp[((unsigned char)*(s + 8))];
            int t1 = domain_temp[((unsigned char)*(s + 9))];
            ++this->list_count_large_[type][hit][t][t1];
        }
        else
        {
            int t = domain_temp[((unsigned char)*(s + 7))];
            int t1 = domain_temp[((unsigned char)*(s + 8))];
            ++this->list_count_large_[type][hit][t][t1];
        }
        s = se + 1;
    }
}

PrefixFilterLargeLoad *PrefixFilterLargeLoad::load_large(char *p, uint64_t size)
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
    PrefixFilterLargeLoad *filter = new PrefixFilterLargeLoad();
    filter->prepare_buf_large(p, size);
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            // for (int k = 0; k < 2; ++k)
            {
                for (int m = 0; m < DOMAIN_CHAR_COUNT; ++m)
                {
                    for (int n = 0; n < DOMAIN_CHAR_COUNT; ++n)
                    {
                        if (filter->list_count_large_[i][j][m][n] > 0)
                        {
                            filter->list_https_large_[i][j][m][n] = (char **)malloc(filter->list_count_large_[i][j][m][n] * sizeof(char *));
                        }
                    }
                }
            }
        }
    }
    filter->load_large_(p, size);
    filter->p_ = p;
    filter->buf_size_ = size;
    return filter;
}

void PrefixFilterLargeLoad::load_large_(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    int count[3][2][DOMAIN_CHAR_COUNT][DOMAIN_CHAR_COUNT] = {0};
    while (s < e)
    {
        char *se = strchr(s, '\n');
        int port_type = 0;
        if (s[0] == '/')
        {
            s += 2;
            port_type = 0; //80 | 443
        }
        else if (s[7] == '/')
        {
            s += 8;
            port_type = 2; // 443
        }
        else
        {
            s += 7;
            port_type = 1; // 80
        }
        s[-1] = port_type;
        int t = domain_temp[((unsigned char)(*s))];
        int t1 = domain_temp[((unsigned char)(*(s + 1)))];
        uint16_t len = se - s - 4 - 2;
        *(uint16_t *)(s) = len;
        uint8_t type = se[-3] == '=' ? 2 : se[-3] == '+' ? 1 : 0;
        bool hit = se[-1] == '-' ? true : false;
        list_https_large_[type][hit][t][t1][count[type][hit][t][t1]] = s;
        ++count[type][hit][t][t1];
#ifdef DEBUG
        *se = '\0';
        printf("PrefixFilter::load_:%d,%s\n", len, s);
#endif
        s = se + 1;
    }
}