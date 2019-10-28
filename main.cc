#include <vector>
#include <string>
#include <algorithm>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <thread>

#include "domain_filter.h"
#include "prefix_filter.h"
#include "url_filter.h"
#include "midfile.h"
#include "util.h"

#include "SP_Stream.h"
#include "Global_Macros.h"
#include "Request.h"
#include "Global.h"

#include "readfile_module.h"
#include "domainload_module.h"
#include "prefixload_module.h"
#include "urlload_module.h"
#include "write_module.h"

#include "readurl1_module.h"
#include "urldp_module.h"
#include "mid_module.h"

#include "readurl2_module.h"
#include "urlpf_module.h"

#include "domainmerge_module.h"
#include "prefixmerge_module.h"

#include "urldp1_module.h"
#include "midlarge_module.h"

#include "prefixloadlarge_module.h"
#include "prefixwrite_module.h"

#include "readurllarge_module.h"
#include "urlpflarge_module.h"
#include "writeurllarge_module.h"

using namespace std;

SP_Stream *s_instance_stream = NULL;
static SP_Module *modules[10];

static void init_temp()
{
    for (int i = 0; i < 256; ++i)
    {
        if (i >= 'a' && i <= 'z') //11-36
        {
            domain_temp[i] = i - 'a' + 11;
        }
        else if (i >= '0' && i <= '9') //1-10
        {
            domain_temp[i] = i - '0' + 1;
        }
        else if (i == '_') //37
        {
            domain_temp[i] = 37;
        }
        else if (i == '-') //0
        {
            domain_temp[i] = 0;
        }
        else
        {
            domain_temp[i] = 38;
        }
    }
}

static void init_temp2()
{
    for (int i = 0; i < 256; ++i)
    {
        if (i >= 'a' && i <= 'z') //14-39
        {
            domain_temp[i] = i - 'a' + 14;
        }
        else if (i >= '0' && i <= '9') //3-12
        {
            domain_temp[i] = i - '0' + 3;
        }
        else if (i == '_') //40
        {
            domain_temp[i] = 40;
        }
        else if (i == '-') //0
        {
            domain_temp[i] = 0;
        }
        else if (i == '.')
        {
            domain_temp[i] = 1;
        }
        else if (i == '/')
        {
            domain_temp[i] = 2;
        }
        else if (i == ':')
        {
            domain_temp[i] = 13;
        }
        else
        {
            domain_temp[i] = 256;
        }
    }
}

static void dumpCounters(FILE *out, const FilterCounters *cc)
{
    fprintf(out, "%d\n", cc->pass);
    fprintf(out, "%d\n", cc->hit);
    fprintf(out, "%d\n", cc->miss);
    //fprintf(out, "%d\n", cc->invalidUrl);
    fprintf(out, "%08x\n", cc->passchecksum);
    fprintf(out, "%08x\n", cc->hitchecksum);
}

int main(int argc, char **argv)
{
    // init_temp();
    init_temp2();
    gEnd = false;

    FilterCounters counters;
    const char *domainFilterPath = argv[1];
    const char *urlPrefixFilterPath = argv[2];
    const char *urlidPath = argv[3];
    const char *urlfeature = argv[4];

    uint64_t domainFilter_size = file_size(domainFilterPath);
    uint64_t urlPrefixFilter_size = file_size(urlPrefixFilterPath);
    uint64_t urlid_size = file_size(urlidPath);

    Request *data = new Request();
    gRequest = data;
    SP_Message_Block_Base *msg = NULL;

    uint64_t size = domainFilter_size;
    FILE *fp;

    int case_type = 2;
    if (domainFilter_size < SMALLSIZE)
    {
        case_type = 0;
    }
    else if (urlid_size < SMALLSIZE)
    {
        case_type = 1;
    }
#if (SP_DEBUG_SWITCH == 1)
    case_type = 2;
#endif

    // if (size < SMALLSIZE)
    if (case_type == 0)
    {
        fp = fopen(domainFilterPath, "r");
        // uint64_t size = sizeoffile(fp);
        SP_DEBUG("fp=%s,size=%lld\n", domainFilterPath, size);
        data->fp_in_ = fp;
        data->length_ = size;
        gStart = false;

        SP_NEW_RETURN(s_instance_stream, SP_Stream, -1);
        SP_NEW_RETURN(modules[0], ReadFile_Module(1), -1);
        SP_NEW_RETURN(modules[1], DomainLoad_Module(8), -1);
        SP_NEW_RETURN(modules[2], DomainMerge_Module(8), -1);

        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }
        SP_NEW_RETURN(msg, SP_Message_Block_Base((SP_Data_Block *)data), -1);
        s_instance_stream->put(msg);
        s_instance_stream->get(msg);
        fclose(fp);
        fp = NULL;
        data->fp_in_ = NULL;
        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->pop();
        }

        SP_DEBUG("prefix begin\n");
        SP_DEBUG("fp=%s,size=%lld\n", urlPrefixFilterPath, size);
        SP_NEW_RETURN(modules[0], ReadFile_Module(1), -1);
        SP_NEW_RETURN(modules[1], PrefixLoad_Module(8), -1);
        SP_NEW_RETURN(modules[2], PrefixMerge_Module(8), -1);

        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }

        fp = fopen(urlPrefixFilterPath, "r");
        // size = file_size(urlPrefixFilterPath);
        size = urlPrefixFilter_size;
        SP_DEBUG("fp=%p,size=%lld\n", fp, size);
        data->fp_in_ = fp;
        data->length_ = size;
        s_instance_stream->put(msg);
        s_instance_stream->get(msg);
        fclose(fp);
        fp = NULL;
        data->fp_in_ = NULL;
        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->pop();
        }

        SP_DEBUG("url begin\n");
        SP_DEBUG("fp=%s,size=%lld\n", urlidPath, size);
        SP_NEW_RETURN(modules[0], ReadFile_Module(1), -1);
        SP_NEW_RETURN(modules[1], UrlLoad_Module(8), -1);
        SP_NEW_RETURN(modules[2], Write_Module(1), -1);

        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }
        fp = fopen(urlidPath, "r");
        // size = file_size(urlidPath);
        size = urlid_size;

        if (fp == NULL)
        {
            fputs("File out error", stderr);
            exit(1);
        }
        SP_DEBUG("fp=%p,size=%lld\n", fp, size);
        data->fp_in_ = fp;
        data->length_ = size;
        data->fp_out_ = stdout;
        data->reset_para();
        s_instance_stream->put(msg);
        s_instance_stream->get(msg);
        dumpCounters(stdout, &data->counter_);
        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->pop();
        }
        fclose(fp);
        delete data;
        SP_DES(msg);
        s_instance_stream->close();
        delete s_instance_stream;
        s_instance_stream = NULL;
    }
    else if (case_type == 1)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        SP_NEW_RETURN(s_instance_stream, SP_Stream, -1);

        SP_DEBUG("url begin\n");
        memset(dp_need, 0, sizeof(bool) * (DOMAIN_CHAR_COUNT));
        memset(dp_sp_need, 0, sizeof(bool) * (DOMAIN_CHAR_COUNT)*2);
        memset(pf_need, 0, sizeof(bool) * (DOMAIN_CHAR_COUNT)*DOMAIN_CHAR_COUNT);

        SP_DEBUG("fp=%s,size=%lld\n", urlidPath, size);
        SP_NEW_RETURN(modules[0], ReadFile_Module(1), -1);
        SP_NEW_RETURN(modules[1], UrlLoad_Module_case2(8), -1);

        for (int i = 1; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }
        fp = fopen(urlidPath, "r");
        // size = file_size(urlidPath);
        size = urlid_size;

        if (fp == NULL)
        {
            fputs("File out error", stderr);
            exit(1);
        }
        SP_DEBUG("fp=%p,size=%lld\n", fp, size);
        data->fp_in_ = fp;
        data->length_ = size;
        data->fp_out_ = stdout;
        data->reset_para();
        SP_NEW_RETURN(msg, SP_Message_Block_Base((SP_Data_Block *)data), -1);
        s_instance_stream->put(msg);
        s_instance_stream->get(msg);
        for (int i = 1; i >= 0; --i)
        {
            s_instance_stream->pop();
        }
        fclose(fp);
        for (int i = 0; i < data->url_filter_list_.size(); ++i)
        {
            for (int j = 0; j < DOMAIN_CHAR_COUNT; ++j)
            {
                if (!dp_need[j] && data->url_filter_list_[i]->list_domainport_count_[j] > 0)
                {
                    dp_need[j] = true;
                }
                for (int k = 0; k < 2; ++k)
                {
                    if (!dp_sp_need[k][j] && data->url_filter_list_[i]->list_domain_sp_count_[k][j] > 0)
                    {
                        dp_sp_need[k][j] = true;
                    }
                }
            }
        }

        //domain file read
        SP_DEBUG("read doamin begin\n");
        fp = fopen(domainFilterPath, "r");
        size = domainFilter_size;
        // uint64_t size = sizeoffile(fp);
        SP_DEBUG("fp=%s,size=%lld\n", domainFilterPath, size);
        data->fp_in_ = fp;
        data->length_ = size;
        data->reset_para();
        gStart = false;

        SP_NEW_RETURN(modules[0], ReadFile_Module(1), -1);
        SP_NEW_RETURN(modules[1], DomainLoad_Module_case2(8), -1);
        SP_NEW_RETURN(modules[2], DomainMerge_Module_case2(8), -1);

        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }
        s_instance_stream->put(msg);
        s_instance_stream->get(msg);
        fclose(fp);
        fp = NULL;
        data->fp_in_ = NULL;
        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->pop();
        }

        ///////urldp
        data->reset_para();
        SP_NEW_RETURN(modules[0], UrlDP_Module(8), -1);

        for (int i = 0; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }
        data->size_split_buf = data->url_filter_list_.size();
        for (int i = 0; i < data->url_filter_list_.size(); ++i)
        {
            data->url_filter_list_[i]->domainfilter_ = data->domain_filter_;
            SP_NEW_RETURN(msg, SP_Message_Block_Base(), -1);
            msg->idx(i);
            s_instance_stream->put(msg);
        }
        s_instance_stream->get(msg);
        delete data->domain_filter_;
        data->domain_filter_ = NULL;
        for (int i = 0; i >= 0; --i)
        {
            s_instance_stream->pop();
        }
        for (int i = 0; i < data->url_filter_list_.size(); ++i)
        {
            for (int j = 0; j < DOMAIN_CHAR_COUNT; ++j)
            {
                for (int k = 0; k < DOMAIN_CHAR_COUNT; ++k)
                {
                    if (data->url_filter_list_[i]->list_count_[j][k] > 0)
                        pf_need[j][k] = true;
                }
            }
        }

        SP_DEBUG("prefix begin\n");
        SP_DEBUG("fp=%s,size=%lld\n", urlPrefixFilterPath, size);
        SP_NEW_RETURN(modules[0], ReadFile_Module(1), -1);
        SP_NEW_RETURN(modules[1], PrefixLoad_Module_case2(8), -1);
        SP_NEW_RETURN(modules[2], PrefixMerge_Module_case2(8), -1);

        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }

        fp = fopen(urlPrefixFilterPath, "r");
        // size = file_size(urlPrefixFilterPath);
        size = urlPrefixFilter_size;
        SP_DEBUG("fp=%p,size=%lld\n", fp, size);
        data->fp_in_ = fp;
        data->length_ = size;
        data->reset_para();
        s_instance_stream->put(msg);
        s_instance_stream->get(msg);
        fclose(fp);
        fp = NULL;
        data->fp_in_ = NULL;
        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->pop();
        }
        //////urlpf
        SP_NEW_RETURN(modules[0], UrlPF_Module(8), -1);

        for (int i = 0; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }
        data->reset_para();
        data->size_split_buf = data->url_filter_list_.size();
        for (int i = 0; i < data->url_filter_list_.size(); ++i)
        {
            data->url_filter_list_[i]->prefixfilter_ = data->prefix_filter_;
            SP_NEW_RETURN(msg, SP_Message_Block_Base(), -1);
            msg->idx(i);
            s_instance_stream->put(msg);
        }
        s_instance_stream->get(msg);
        for (int i = 0; i < data->url_filter_list_.size(); ++i)
        {
            UrlFilter *filter = data->url_filter_list_[i];
            filter->write_tag(stdout);
            data->counter_.pass += filter->counters_.pass;
            data->counter_.hit += filter->counters_.hit;
            data->counter_.miss += filter->counters_.miss;
            data->counter_.passchecksum ^= filter->counters_.passchecksum;
            data->counter_.hitchecksum ^= filter->counters_.hitchecksum;
        }
        dumpCounters(stdout, &data->counter_);

        delete data;
        SP_DES(msg);
        s_instance_stream->close();
        delete s_instance_stream;
        s_instance_stream = NULL;
        gettimeofday(&t2, 0);
        SP_DEBUG("main=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
    }
    else //2
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        //domain file read
        SP_DEBUG("read doamin begin\n");
        fp = fopen(domainFilterPath, "r");
        // uint64_t size = sizeoffile(fp);
        SP_DEBUG("fp=%p,size=%lld\n", fp, size);
        data->fp_in_ = fp;
        data->length_ = size;

        SP_NEW_RETURN(s_instance_stream, SP_Stream, -1);
        SP_NEW_RETURN(modules[0], ReadFile_Module_v1(1), -1);
        SP_NEW_RETURN(modules[1], DomainLoad_Module_v1(8), -1);
        SP_NEW_RETURN(modules[2], DomainMerge_Module_v1(8), -1);

        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }
        SP_NEW_RETURN(msg, SP_Message_Block_Base((SP_Data_Block *)data), -1);
        s_instance_stream->put(msg);
        s_instance_stream->get(msg);
        fclose(fp);
        fp = NULL;
        data->fp_in_ = NULL;
        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->pop();
        }
        for (uint i = 0; i < data->domain_filter_list_.size(); ++i)
        {
            DomainFilter *filter = data->domain_filter_list_[i];
            delete (filter);
        }
        data->domain_filter_list_.clear();
        SP_DEBUG("read doamin end\n");

        //url file read and domain filter
        SP_DEBUG("init urlfilter begin\n");
        uint64_t split_mem = 0;
        uint64_t max_file_size = domainFilter_size;
        if (max_file_size + urlid_size < MAX_USE_MEM_SIZE)
        {
            // split_mem = ceil(urlid_size / 16) + 64;
            split_mem = ceil(urlid_size / 8) + 64;
        }
        else
        {
            // split_mem = ceil((MAX_USE_MEM_SIZE - max_file_size) / 8 + SMALLFILESIZE);
            split_mem = FILESPLITSIZE;
        }
        //prepare urlfilter
        {
            for (int i = 0; i < 16; ++i)
            {
                UrlFilter *filter = new UrlFilterLarge();
                filter->buf_size_ = split_mem;
                filter->p_ = (char *)malloc(filter->buf_size_ + BUFHEADSIZE);
                filter->p_ = filter->p_ + BUFHEADSIZE;
                // filter->max_list_domainport_count_ = INITURLCOUNT;
                // filter->list_domainport_ = (DomainPortBuf *)malloc(filter->max_list_domainport_count_ * sizeof(DomainPortBuf));
                // filter->max_list_count_ = INITURLCOUNT;
                // filter->list_ = (char **)malloc(filter->max_list_count_ * sizeof(char *));
                gQueue.push(filter);
            }
        }
        SP_DEBUG("init urlfilter end\n");

        SP_DEBUG("url1 begin\n");
        SP_NEW_RETURN(modules[0], ReadUrl1_Module(1), -1);
        SP_NEW_RETURN(modules[1], UrlDP1_Module(8), -1);
        SP_NEW_RETURN(modules[2], MidLarge_Module(1), -1);

        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }
        size = urlid_size;

        fp = fopen(urlidPath, "r");
        assert(fp);
        data->fp_in_ = fp;
        data->length_ = size;
        data->fp_out_ = stdout;
        data->reset_para();
        data->mid_file_ = new MidFile();
        data->mid_file_->init(FILESPLITSIZE);
        data->split_size_ = split_mem;
        s_instance_stream->put(msg);
        s_instance_stream->get(msg);
        fclose(fp);
        delete (DomainFilterMerge *)(data->domain_filter_);
        data->domain_filter_ = NULL;
        int count = (int)gQueue.size();
        SP_DEBUG("count=%d\n", count);
        UrlFilterLarge *f = NULL;
        while (!gQueue.empty())
        {
            f = (UrlFilterLarge *)(gQueue.front());
            gQueue.pop();
            // f->clear_domain_list();
            delete f;
            // gQueue.push(f);
            count--;
            SP_DEBUG("gQueue->clear_domain_list\n");
        }

        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->pop();
        }
        SP_DEBUG("url2 end\n");

        //prefix filter load
        SP_DEBUG("read prefix begin\n");
        SP_NEW_RETURN(modules[0], ReadFile_Module(1), -1);
        SP_NEW_RETURN(modules[1], PrefixLoadLarge_Module(8), -1);
        SP_NEW_RETURN(modules[2], PrefixWrite_Module(1), -1);

        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }

        fp = fopen(urlPrefixFilterPath, "r");
        assert(fp);
        // size = file_size(urlPrefixFilterPath);
        size = urlPrefixFilter_size;
        SP_DEBUG("fp=%p,size=%lld\n", fp, size);
        data->fp_in_ = fp;
        data->length_ = size;
        data->reset_para();
        data->mid_file_->uninit();
        data->mid_file_->init(256 * 1024 * 1024);
        s_instance_stream->put(msg);
        s_instance_stream->get(msg);
        SP_DES(msg);
        fclose(fp);
        fp = NULL;
        data->fp_in_ = NULL;
        data->mid_file_->uninit();
        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->pop();
        }
        SP_DEBUG("read prefix end\n");

        //tmp url read and prefix filter
        SP_DEBUG(" prefix filter begin\n");
        SPFFilter *spf_filter;
        SPFFilter *list_spf_filter_[9];
        for (int i = 0; i < 9; ++i)
        {
            spf_filter = new SPFFilter();
            // gQueueFilter.push(url_pf_filter);
            list_spf_filter_[i] = spf_filter;
            gQUrlPfTask.push(spf_filter);
        }
        data->reset_para();
        SP_NEW_RETURN(modules[0], ReadUrlLarge_Module(1), -1);
        SP_NEW_RETURN(modules[1], UrlPFLarge_Module(8), -1);
        // SP_NEW_RETURN(modules[2], WriteUrlLarge_Module(1), -1);
        modules[2] = WriteUrlLarge_Module::instance();

        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }

        SUrlFilter *surl_filter;
        data->fp_out_ = stdout;

        SUrlFilter *list_surl_filter_[16];
        for (int i = 0; i < 16; ++i)
        {
            surl_filter = new SUrlFilter();
            // gQueueFilter.push(url_pf_filter);
            list_surl_filter_[i] = surl_filter;
            SP_NEW_RETURN(msg, SP_Message_Block_Base((SP_Data_Block *)surl_filter), -1);
            s_instance_stream->put(msg);
        }

        // s_instance_stream->put(msg);
        s_instance_stream->get(msg);
        for (int i = 0; i < 16; ++i)
        {
            surl_filter = list_surl_filter_[i];
            // gQueueFilter.pop();
            data->counter_.pass += surl_filter->counters_.pass;
            data->counter_.hit += surl_filter->counters_.hit;
            data->counter_.miss += surl_filter->counters_.miss;
            data->counter_.passchecksum ^= surl_filter->counters_.passchecksum;
            data->counter_.hitchecksum ^= surl_filter->counters_.hitchecksum;
            delete surl_filter;
            if (i < 9)
                delete list_spf_filter_[i];
        }
        dumpCounters(stdout, &data->counter_);
        gEnd = true;
        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->pop();
        }
        SP_DEBUG(" prefix filter end\n");
        delete data;
        SP_DES(msg);
        s_instance_stream->close();
        delete s_instance_stream;
        s_instance_stream = NULL;
        SP_DEBUG("gQueueCache:%zu,gQueue:%zu\n", gQueueCache.size(), gQueue.size());
        gettimeofday(&t2, 0);
        SP_DEBUG("main=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
    }
    return 0;
}
