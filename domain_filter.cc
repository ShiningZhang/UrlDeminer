#include "domain_filter.h"

#include <stdlib.h>
#include <assert.h>

#include "util.h"
#include "pdqsort.h"

#include "Global_Macros.h"

using namespace std;

bool unique_dp_port(const DomainPortBuf &e1, const DomainPortBuf &e2);

DomainFilter::DomainFilter()
{
    memset(list_, 0, sizeof(DomainPortBuf *) * DOMAIN_CHAR_COUNT);
    memset(list_count_, 0, sizeof(int) * DOMAIN_CHAR_COUNT);
    p_ = NULL;
    size_ = 0;
    memset(list_range_, 0, sizeof(int *) * DOMAIN_CHAR_COUNT);

    list_port_ = NULL;
    list_port_count_ = 0;

    memset(list_c, 0, 2 * sizeof(int) * DOMAIN_CHAR_COUNT);

    memset(list_sp_, 0, sizeof(DomainPortBuf *) * DOMAIN_CHAR_COUNT * 2);
    memset(list_sp_count_, 0, sizeof(int) * DOMAIN_CHAR_COUNT * 2);
    memset(list_sp_range_, 0, sizeof(int *) * DOMAIN_CHAR_COUNT * 2);
    memset(list_sp_c_, 0, 2 * 2 * sizeof(int));
    memset(list_sp_cc_, 0, 2 * 2 * sizeof(int) * DOMAIN_CHAR_COUNT);
}

DomainFilter::~DomainFilter()
{
    SP_DEBUG("~DomainFilter");
    if (p_ != NULL)
    {
        p_ = p_ - BUFHEADSIZE;
        free(p_);
        p_ = NULL;
    }
    for (int k = 0; k < DOMAIN_CHAR_COUNT; ++k)
    {
        if (list_range_[k] != NULL)
        {
            free(list_range_[k]);
            list_range_[k] = NULL;
        }
        if (list_[k] != NULL)
        {
            free(list_[k]);
            list_[k] = NULL;
        }
    }
    if (list_port_ != NULL)
    {
        free(list_port_);
        list_port_ = NULL;
    }
    for (int j = 0; j < 2; ++j)
    {
        for (int k = 0; k < DOMAIN_CHAR_COUNT; ++k)
        {
            if (list_sp_range_[j][k] != NULL)
            {
                free(list_sp_range_[j][k]);
                list_sp_range_[j][k] = NULL;
            }
            if (list_sp_[k] != NULL)
            {
                free(list_sp_[j][k]);
                list_sp_[j][k] = NULL;
            }
        }
    }
}

void DomainFilter::prepare_buf(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        char *offset = se - 3;
        int t = domain_temp[(int)((unsigned char)(*offset))];
        while (offset > s && offset > (se - 9))
        {
            if (*offset != ':')
                --offset;
            else
                break;
        }
        if (*offset == ':')
        {
            t = domain_temp[(int)((unsigned char)(*(offset - 1)))];
            ++this->list_port_count_;
        }
        else
        {
            offset = se - 3;
            if (memcmp(offset - 3, ".com", 4) == 0)
            {
                if (offset - 4 > s)
                    list_sp_count_[0][domain_temp[(unsigned char)(*(offset - 4))]]++;
            }
            else if (memcmp(offset - 2, ".cn", 3) == 0)
            {
                if (offset - 3 > s)
                    list_sp_count_[1][domain_temp[(unsigned char)(*(offset - 3))]]++;
            }
            else
                ++this->list_count_[t];
        }
        // LOG("list_count_[%d][%d]=%d\n", se[-1] == '-',  t, list_count_[se[-1] == '-'][t]);
        s = se + 1;
    }
}

/* void DomainFilter::load_(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    DomainPortBuf b;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        int len = se - s;
        b.hit = (int)(s[len - 1] == '-');
        s[len - 2] = '\0';
        char *port = strchr(s, ':');
        if (port != NULL)
        {
            b.port = atoi(port + 1);
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
#ifdef DEBUG
        printf("load_:domain:%10s,n:%d,port:%d,hit:%d\n", b.start, b.n, b.port, b.hit);
#endif
        s = se + 1;
    }
} */

void DomainFilter::load_(char *p, uint64_t size)
{
    char *e = p + size;
    char *s = p;
    int count[DOMAIN_CHAR_COUNT] = {0};
    int count_sp[2][DOMAIN_CHAR_COUNT] = {0};
    int count_port = 0;
    DomainPortBuf b;
    while (s < e)
    {
        char *se = strchr(s, '\n');
        int len = se - s - 2;
        char *offset = se - 3;
        int port = 0;
        int t = domain_temp[(int)((unsigned char)(*offset))];
        bool hit = se[-1] == '-';
        while (offset > s && offset > (se - 9))
        {
            if (*offset != ':')
                --offset;
            else
                break;
        }
        if (*offset == ':')
        {
            port = atoi(offset + 1);
            // port = *(uint16_t *)(offset + 1);
            len = offset - s;
            t = domain_temp[(int)((unsigned char)(*(offset - 1)))];
            b.port = port;
            b.hit = hit;
            b.n = len;
            b.start = s;
            this->list_port_[count_port++] = b;
            // *(uint16_t *)(s - 2) = (uint16_t)(len);
            // assert(len);
            // assert(len != 1);
#ifdef DEBUG
            printf("DomainFilter::load_:hit=%d,port=%d,n=%d,%s\n", b.hit, b.port, b.n, s);
#endif
        }
        else
        {
            b.hit = hit;
            b.start = s;
            b.port = 0;
            if (len >= 4 && memcmp(se - 6, ".com", 4) == 0)
            {
                offset = se - 6;
                {
                    if (len > 5)
                    {
                        int t1 = domain_temp[(unsigned char)(*(offset - 1))];
                        b.n = len - 5;
                        list_sp_[0][t1][count_sp[0][t1]++] = b;
                    }
                    else if (len == 5)
                    {
                        list_sp_cc_[0][hit][domain_temp[(unsigned char)(*(offset - 1))]]++;
                    }
                    else if (len == 4)
                    {
                        list_sp_c_[0][hit]++;
                        // assert(0);
                    }
                }
            }
            else if (len >= 3 && memcmp(se - 5, ".cn", 3) == 0)
            {
                offset = se - 5;
                {
                    if (len > 4)
                    {
                        int t1 = domain_temp[(unsigned char)(*(offset - 1))];
                        b.n = len - 4;
                        list_sp_[1][t1][count_sp[1][t1]++] = b;
                    }
                    else if (len == 4)
                    {
                        list_sp_cc_[1][hit][domain_temp[(unsigned char)(*(offset - 1))]]++;
                    }
                    else if (len == 3)
                    {
                        list_sp_c_[1][hit]++;
                        // assert(0);
                    }
                }
            }
            else
            {
                b.n = len - 1;
                // *(uint16_t *)(s - 2) = (uint16_t)(len - 1);
                // assert(len != 1);
                if (len == 1)
                {
                    ++list_c[hit][t];
                }
                else
                {
                    this->list_[t][count[t]] = b;
                    ++count[t];
                }
                if (len == 3 && memcmp(s, "com", 3) == 0)
                {
                    ++list_sp_c_[0][hit];
                    // assert(0);
                }
                else if (len == 2 && memcmp(s, "cn", 2) == 0)
                {
                    ++list_sp_c_[1][hit];
                    // assert(0);
                }
            }
#ifdef DEBUG
            printf("DomainFilter::load_:hit=%d,n=%d,t=%d,%s\n", hit, len - 1, t, s);
#endif
        }
        s = se + 1;
    }
    memcpy(list_count_, count, DOMAIN_CHAR_COUNT * sizeof(int));
    list_port_count_ = count_port;
    memcpy(list_sp_count_, count_sp, 2 * DOMAIN_CHAR_COUNT * sizeof(int));
}

void prepare_range(DomainPortBuf *list, int size, int *&range)
{
    if (size == 0)
        return;
    range = (int *)malloc(size * sizeof(int));
    DomainPortBuf *pa = list;
    range[0] = 1;
    for (int i = 1; i < size; ++i)
    {
        DomainPortBuf *pb = list + i;
        if (compare_dp_eq(pa, pb) != 0)
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

void DomainFilter::init_list()
{
    for (int k = 0; k < DOMAIN_CHAR_COUNT; ++k)
    {
        if (list_count_[k] > 0)
        {
            list_[k] = (DomainPortBuf *)malloc(list_count_[k] * sizeof(DomainPortBuf));
#ifdef DEBUG
            printf("DomainFilter::load:[%d,%d],count=%d\n", i, k, list_count_[i][k]);
#endif
        }
    }
    if (list_port_count_ > 0)
    {
        list_port_ = (DomainPortBuf *)malloc(list_port_count_ * sizeof(DomainPortBuf));
#ifdef DEBUG
        printf("DomainFilter::load:,count=%d\n", list_port_count_);
#endif
    }
    for (int j = 0; j < 2; ++j)
    {
        for (int k = 0; k < DOMAIN_CHAR_COUNT; ++k)
        {
            if (list_sp_count_[j][k] > 0)
            {
                list_sp_[j][k] = (DomainPortBuf *)malloc(list_sp_count_[j][k] * sizeof(DomainPortBuf));
#ifdef DEBUG
                printf("DomainFilter::load:[%d,%d],count=%d\n", j, k, list_sp_count_[j][k]);
#endif
            }
        }
    }
}

DomainFilter *DomainFilter::load(char *p, uint64_t size)
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
    DomainFilter *filter = new DomainFilter();
    filter->prepare_buf(p, size);
    filter->init_list();

    filter->load_(p, size);
    filter->p_ = p;
    filter->size_ = size;
    for (int m = 0; m < DOMAIN_CHAR_COUNT; ++m)
    {
        if (filter->list_count_[m] > 0)
        {
            pdqsort(filter->list_[m], filter->list_[m] + filter->list_count_[m], compare_dp);
            // prepare_range(filter->list_[i][m], filter->list_count_[i][m], filter->list_range_[i][m]);
#ifdef DEBUG
            for (int k = 0; k < filter->list_count_[m]; ++k)
            {
                printf("domain:%s,hit:%d\n", filter->list_[m][k].start, filter->list_[m][k].hit);
            }
#endif
        }
    }
    for (int j = 0; j < 2; ++j)
    {
        for (int m = 0; m < DOMAIN_CHAR_COUNT; ++m)
        {
            if (filter->list_sp_count_[j][m] > 0)
            {
                pdqsort(filter->list_sp_[j][m], filter->list_sp_[j][m] + filter->list_sp_count_[j][m], compare_dp);
                // prepare_range(filter->list_sp_[i][j][m], filter->list_sp_count_[i][j][m], filter->list_sp_range_[i][j][m]);
#ifdef DEBUG
                for (int k = 0; k < filter->list_sp_count_[j][m]; ++k)
                {
                    printf("domain:%s,hit:%d\n", filter->list_sp_[j][m][k], filter->list_sp_[j][m][k].hit);
                }
#endif
            }
        }
    }
    return filter;
}

#ifdef VER1
int DomainFilterMerge::merge(vector<DomainFilter *> domain_filter_list, int i, int type)
{
    if (type >= DOMAIN_CHAR_COUNT)
    {
        return merge_port(domain_filter_list);
    }
    int count = 0;
    for (int i = 0; i < 2; ++i)
    {
        {
            int size = 0;
            for (size_t k = 0; k < domain_filter_list.size(); ++k)
            {
                size += domain_filter_list[k]->list_count_[i][type];
            }
            if (size > 0)
            {
                count += size;
                list_[i][type] = (char **)malloc(size * sizeof(char *));
                list_count_[i][type] = size;
                size = 0;
                for (size_t k = 0; k < domain_filter_list.size(); ++k)
                {
                    if (domain_filter_list[k]->list_count_[i][type] != 0)
                    {
                        memcpy(list_[i][type] + size, domain_filter_list[k]->list_[i][type], domain_filter_list[k]->list_count_[i][type] * sizeof(char *));
                        size += domain_filter_list[k]->list_count_[i][type];
                    }
                }
                pdqsort(list_[i][type], list_[i][type] + list_count_[i][type], compare_dp_char);
                prepare_range(list_[i][type], list_count_[i][type], list_range_[i][type]);
            }
        }
#ifdef DEBUG
        for (int tmp = 0; tmp < list_count_[i][type]; ++tmp)
        {
            printf("DomainFilterMerge::mergehit=%d,port=0,type=%d,%s\n", i, type, list_[i][type][tmp]);
        }
#endif
    }
    return count;
}
#endif

struct HeapItemDp
{
    int ed_;
    int idx_;
    int start_;
    DomainPortBuf *list_;
    inline bool operator<(const HeapItemDp &y) const
    {
        if (idx_ == 0 || y.idx_ == 0)
            return idx_ < y.idx_;
        return compare_dp(list_[start_], y.list_[y.start_]);
    }
};

typedef HeapItemDp *pHeapItemDp;

HeapItemDp MinHI = {0, 0, 0, 0};

static inline void LoserAdjust(int *ls, const HeapItemDp *arr, int s, const int n)
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

static inline void LoserBuild(int *ls, const HeapItemDp *arr, const int n)
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

int DomainFilterMerge::merge(vector<DomainFilter *> domain_filter_list, int i, int type)
{
    if (type >= DOMAIN_CHAR_COUNT && i < 1)
    {
        return merge_port(domain_filter_list);
    }
    if (i >= 1 && type >= DOMAIN_CHAR_COUNT)
    {
        return 0;
    }
    int count = 0;
    // for (int i = 0; i < 2; ++i)
    if (i < 1)
    {
        {
            int size = 0;
            for (size_t k = 0; k < domain_filter_list.size(); ++k)
            {
                size += domain_filter_list[k]->list_count_[type];
                // SP_DEBUG("k=%d,size=%d,list=%p\n", k, domain_filter_list[k]->list_count_[i][type], domain_filter_list[k]->list_[i][type]);
            }
            if (size > 0)
            {
                count += size;
                list_[type] = (DomainPortBuf *)malloc(size * sizeof(DomainPortBuf));
                list_count_[type] = size;
                size = 0;
                {
                    size_t part_num = domain_filter_list.size();
                    int *loser = (int *)malloc((2 * part_num + 1) * sizeof(int));
                    int sl = 0;
                    HeapItemDp *heap = (HeapItemDp *)malloc((part_num + 1) * sizeof(HeapItemDp));
                    memset(heap, 0, (part_num + 1) * sizeof(HeapItemDp));
                    for (uint k = 0; k < part_num; ++k)
                    {
                        if (domain_filter_list[k]->list_count_[type] > 0)
                        {
                            // HeapItemDp *pHI = heap + sl++;
                            heap[sl].ed_ = domain_filter_list[k]->list_count_[type] - 1;
                            heap[sl].start_ = 0;
                            heap[sl].list_ = domain_filter_list[k]->list_[type];
                            heap[sl].idx_ = part_num;
                            // SP_DEBUG("k=%d,sl=%d,ed=%d,list=%p\n", k, sl, heap[sl].ed_, heap[sl].list_);
                            ++sl;
                        }
                    }
                    if (sl > 0)
                    {
                        if (sl > 1)
                        {
                            heap[sl] = MinHI;
                            LoserBuild(loser, heap, sl);
                            while (sl > 1)
                            {
                                int win = loser[0];
                                pHeapItemDp pHI = heap + win;
                                list_[type][size++] = pHI->list_[pHI->start_];
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
                                        heap[sl] = MinHI;
                                        LoserBuild(loser, heap, sl);
                                    }
                                }
                            }
                        }
                        if (sl == 1)
                        {
                            pHeapItemDp pHI = heap + 0;
                            memcpy(list_[type] + size, pHI->list_ + pHI->start_, (pHI->ed_ - pHI->start_ + 1) * sizeof(DomainPortBuf));
                        }
                    }
                    free(loser);
                    free(heap);
                }
                DomainPortBuf *last = unique(list_[type], list_[type] + list_count_[type], unique_dp_port);
                size = last - list_[type];
                list_count_[type] = size;
                prepare_range(list_[type], list_count_[type], list_range_[type]);
                for (size_t k = 0; k < domain_filter_list.size(); ++k)
                {
                    if (domain_filter_list[k]->list_[type] != NULL)
                    {
                        free(domain_filter_list[k]->list_[type]);
                        domain_filter_list[k]->list_[type] = NULL;
                    }
                    domain_filter_list[k]->list_count_[type] = 0;
                }
            }
        }
#ifdef DEBUG
        for (int tmp = 0; tmp < list_count_[type]; ++tmp)
        {
            printf("DomainFilterMerge::mergehit=%d,port=0,type=%d,%s\n", list_[type][tmp].hit, type, list_[type][tmp]);
        }
#endif
    }
    else
    {
        int num1 = i - 1;
        {
            int size = 0;
            for (size_t k = 0; k < domain_filter_list.size(); ++k)
            {
                size += domain_filter_list[k]->list_sp_count_[num1][type];
                for (int num2 = 0; num2 < 2; ++num2)
                    list_sp_cc_[num1][num2][type] += domain_filter_list[k]->list_sp_cc_[num1][num2][type];
                // SP_DEBUG("k=%d,size=%d,list=%p\n", k, domain_filter_list[k]->list_count_[i][type], domain_filter_list[k]->list_[i][type]);
            }
            if (size > 0)
            {
                count += size;
                list_sp_[num1][type] = (DomainPortBuf *)malloc(size * sizeof(DomainPortBuf));
                list_sp_count_[num1][type] = size;
                size = 0;
                {
                    size_t part_num = domain_filter_list.size();
                    int *loser = (int *)malloc((2 * part_num + 1) * sizeof(int));
                    int sl = 0;
                    HeapItemDp *heap = (HeapItemDp *)malloc((part_num + 1) * sizeof(HeapItemDp));
                    memset(heap, 0, (part_num + 1) * sizeof(HeapItemDp));
                    for (uint k = 0; k < part_num; ++k)
                    {
                        if (domain_filter_list[k]->list_sp_count_[num1][type] > 0)
                        {
                            // HeapItemDp *pHI = heap + sl++;
                            heap[sl].ed_ = domain_filter_list[k]->list_sp_count_[num1][type] - 1;
                            heap[sl].start_ = 0;
                            heap[sl].list_ = domain_filter_list[k]->list_sp_[num1][type];
                            heap[sl].idx_ = part_num;
                            // SP_DEBUG("k=%d,sl=%d,ed=%d,list=%p\n", k, sl, heap[sl].ed_, heap[sl].list_);
                            ++sl;
                        }
                    }
                    if (sl > 0)
                    {
                        if (sl > 1)
                        {
                            heap[sl] = MinHI;
                            LoserBuild(loser, heap, sl);
                            while (sl > 1)
                            {
                                int win = loser[0];
                                pHeapItemDp pHI = heap + win;
                                list_sp_[num1][type][size++] = pHI->list_[pHI->start_];
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
                                        heap[sl] = MinHI;
                                        LoserBuild(loser, heap, sl);
                                    }
                                }
                            }
                        }
                        if (sl == 1)
                        {
                            pHeapItemDp pHI = heap + 0;
                            memcpy(list_sp_[num1][type] + size, pHI->list_ + pHI->start_, (pHI->ed_ - pHI->start_ + 1) * sizeof(DomainPortBuf));
                        }
                    }
                    free(loser);
                    free(heap);
                }
                DomainPortBuf *last = unique(list_sp_[num1][type], list_sp_[num1][type] + list_sp_count_[num1][type], unique_dp_port);
                size = last - list_sp_[num1][type];
                list_sp_count_[num1][type] = size;
                prepare_range(list_sp_[num1][type], list_sp_count_[num1][type], list_sp_range_[num1][type]);
                for (size_t k = 0; k < domain_filter_list.size(); ++k)
                {
                    if (domain_filter_list[k]->list_sp_[num1][type] != NULL)
                    {
                        free(domain_filter_list[k]->list_sp_[num1][type]);
                        domain_filter_list[k]->list_sp_[num1][type] = NULL;
                    }
                    domain_filter_list[k]->list_sp_count_[num1][type] = 0;
                }
            }
        }
#ifdef DEBUG
        for (int tmp = 0; tmp < list_sp_count_[num1][type]; ++tmp)
        {
            printf("DomainFilterMerge::mergehit=%d,port=0,type=%d,%s\n", list_sp_[num1][tmp].hit, type, list_sp_[num1][tmp]);
        }
#endif
    }
    /* for (size_t k = 0; k < domain_filter_list.size(); ++k)
    {
        if (domain_filter_list[k]->list_[i][type] != NULL)
        {
            free(domain_filter_list[k]->list_[i][type]);
            domain_filter_list[k]->list_[i][type] = NULL;
            domain_filter_list[k]->list_count_[i][type] = 0;
        }
    } */
    return count;
}

bool unique_dp_port(const DomainPortBuf &e1, const DomainPortBuf &e2)
{
    if (e1.port != e2.port || e1.n != e2.n)
        return false;
    int ret = cmpbuf_dp(e1.start, e1.n, e2.start, e2.n);
    return ret == 0;
}

int DomainFilterMerge::merge_port(vector<DomainFilter *> domain_filter_list)
{
    int count = 0;
    // for (int i = 0; i < 2; ++i)
    {
        int size = 0;
        for (size_t k = 0; k < domain_filter_list.size(); ++k)
        {
            size += domain_filter_list[k]->list_port_count_;
        }
        if (size > 0)
        {
            count += size;
            list_port_ = (DomainPortBuf *)malloc(size * sizeof(DomainPortBuf));
            list_port_count_ = size;
            size = 0;
            for (size_t k = 0; k < domain_filter_list.size(); ++k)
            {
                if (domain_filter_list[k]->list_port_count_ != 0)
                {
                    memcpy(list_port_ + size, domain_filter_list[k]->list_port_, domain_filter_list[k]->list_port_count_ * sizeof(DomainPortBuf));
                    size += domain_filter_list[k]->list_port_count_;
                    /* free(domain_filter_list[k]->list_port_[i]);
                    domain_filter_list[k]->list_port_[i] = NULL;
                    domain_filter_list[k]->list_port_count_[i] = 0; */
                }
            }
            pdqsort(list_port_, list_port_ + list_port_count_, compare_dp);
            DomainPortBuf *last = unique(list_port_, list_port_ + list_port_count_, unique_dp_port);
            size = last - list_port_;
            list_port_count_ = size;
            {
                int port = 0;
                DomainPortBuf *list = list_port_;
                for (int m = 0; m < size; ++m)
                {
                    DomainPortBuf *p = list + m;
                    if (port != p->port)
                    {
                        port = p->port;
                        port_start_[port] = p;
                        ++port_size_[port];
                    }
                    else
                    {
                        ++port_size_[port];
                    }
                }
            }
            for (int port = 0; port < 65536; ++port)
            {
                if (port_size_[port] > 0)
                {
                    port_range_[port] = (int *)malloc(port_size_[port] * sizeof(int));
                    DomainPortBuf *pa = port_start_[port];
                    port_range_[port][0] = 1;
                    for (int m = 1; m < port_size_[port]; ++m)
                    {
                        DomainPortBuf *pb = port_start_[port] + m;
                        if (compare_dp_eq(pa, pb) != 0)
                        {
                            pa = pb;
                            port_range_[port][m] = 1;
                        }
                        else
                        {
                            port_range_[port][m] = port_range_[port][m - 1] + 1;
                        }
                    }
                }
            }
        }
#ifdef DEBUG
        for (int tmp = 0; tmp < list_port_count_; ++tmp)
        {
            printf("DomainFilterMerge::merge_port:hit=%d,port=%d,n=%d,%s\n", list_port_[tmp].hit, list_port_[tmp].port, list_port_[tmp].n, list_port_[tmp].start);
        }
#endif
    }
    return count;
}

DomainFilterMerge::DomainFilterMerge()
{
    memset(port_start_, 0, 65536 * sizeof(DomainPortBuf *));
    memset(port_size_, 0, 65536 * sizeof(int));
    memset(port_range_, 0, 65536 * sizeof(int *));
}

DomainFilterMerge::~DomainFilterMerge()
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
    {
        for (int j = 0; j < 65536; ++j)
        {
            if (port_range_[j] != NULL)
            {
                free(port_range_[j]);
            }
        }
    }
}

void DomainFilterMerge::cpy_filter_list(vector<DomainFilter *> &list)
{
    for (uint i = 0; i < list.size(); ++i)
    {
        for (int num1 = 0; num1 < 2; ++num1)
        {
            for (int num2 = 0; num2 < 2; ++num2)
            {
                list_sp_c_[num1][num2] += list[i]->list_sp_c_[num1][num2];
            }
        }
        p_list_.push_back(list[i]->p_ - BUFHEADSIZE);
        buf_size_list_.push_back(list[i]->size_);
        list[i]->p_ = NULL;
        list[i]->size_ = 0;
        delete list[i];
    }
    list.clear();
}

void DomainFilter::compress()
{
    if (size_ == 0)
        return;
    uint64_t size = 0;
    for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
    {
        for (int j = 0; j < list_count_[i]; ++j)
        {
            size += list_[i][j].n;
        }
    }
    for (int i = 0; i < list_port_count_; ++i)
    {
        size += list_port_[i].n;
    }
    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < DOMAIN_CHAR_COUNT; ++j)
        {
            for (int k = 0; k < list_sp_count_[i][j]; ++k)
            {
                size += list_sp_[i][j][k].n;
            }
        }
    }
    char *p = (char *)malloc(size * sizeof(char));
    size = 0;
    for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
    {
        for (int j = 0; j < list_count_[i]; ++j)
        {
            memcpy(p + size, list_[i][j].start, list_[i][j].n);
            list_[i][j].start = p + size;
            size += list_[i][j].n;
        }
    }
    for (int i = 0; i < list_port_count_; ++i)
    {
        memcpy(p + size, list_port_[i].start, list_port_[i].n);
        list_port_[i].start = p + size;
        size += list_port_[i].n;
    }
    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < DOMAIN_CHAR_COUNT; ++j)
        {
            for (int k = 0; k < list_sp_count_[i][j]; ++k)
            {
                memcpy(p + size, list_sp_[i][j][k].start, list_sp_[i][j][k].n);
                list_sp_[i][j][k].start = p + size;
                size += list_sp_[i][j][k].n;
            }
        }
    }
    if (p_ != NULL)
    {
        p_ = p_ - BUFHEADSIZE;
        free(p_);
        p_ = NULL;
    }
    p_ = p + BUFHEADSIZE;
    size_ = size;
}