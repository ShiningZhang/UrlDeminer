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

using namespace std;

static SP_Stream *s_instance_stream = NULL;
static SP_Module *modules[10];

static void init_temp()
{
    for (int i = 0; i < 256; ++i)
    {
        if (i >= 'a' && i <= 'z')
        {
            domain_temp[i] = i - 'a';
        }
        else if (i >= '0' && i <= '9')
        {
            domain_temp[i] = i - '0' + 26;
        }
        else if (i == '_')
        {
            domain_temp[i] = 26 + 10;
        }
        else if (i == '-')
        {
            domain_temp[i] = 26 + 10 + 1;
        }
        else
        {
            domain_temp[i] = 38;
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
    init_temp();

    FilterCounters counters;
    const char *domainFilterPath = argv[1];
    const char *urlPrefixFilterPath = argv[2];
    const char *urlidPath = argv[3];
    const char *urlfeature = argv[4];

    Request *data = new Request();
    SP_Message_Block_Base *msg = NULL;

    uint64_t size = file_size(domainFilterPath);
    FILE *fp;

    // if (size < SMALLSIZE)
    if (true)
    {
        fp = fopen(domainFilterPath, "r");
        // uint64_t size = sizeoffile(fp);
        SP_DEBUG("fp=%p,size=%lld\n", fp, size);
        data->fp_in_ = fp;
        data->length_ = size;

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
        /* 
        {
            SP_DEBUG("test begin\n");
            uint64_t test_size = file_size(urlidPath);
            // FILE *test_fp = fopen(urlidPath, "r");
            int test_fp = open(urlidPath, O_RDONLY | O_LARGEFILE);

            SP_DEBUG("urlidPath=%s\n", urlidPath);
            int l = 1024;
            char tmp[l];
            // fgets(tmp, l, test_fp);
            // SP_DEBUG("%s\n", tmp);
            // rewind(test_fp);
            uint32_t size1 = 0;
            uint64_t begin = 0;
            size_t line_size = test_size < 1024 * 8 ? test_size : ceil((double)test_size / 8) + 64;
            SP_DEBUG("ReadFile_Module:length=%d,line_size=%d\n", test_size, line_size);
            int count = 0;
            while (begin < test_size)
            {
                if (begin + line_size > test_size)
                    line_size = size - begin;
                char *buf = (char *)malloc(size_t(line_size + BUFHEADSIZE));
                if (buf == NULL)
                {
                    fputs("Memory error", stderr);
                    exit(2);
                }
                buf = buf + BUFHEADSIZE;
                // flockfile(data->fp_in_);
                SP_DEBUG("fp=%p,buf=%p,line_size=%zu\n", test_fp, buf, line_size);
                // size1 = fread(buf, sizeof(char), line_size, fp);
                // size1 = readcontent(test_fp, buf, line_size);
                size1 = read(test_fp, buf, line_size);
                SP_DEBUG("size=%d,buf=%s\n", size1, buf);
                begin += size1;
                count++;
                SP_DEBUG("count=%lld\n", count);
            }
            // rewind(test_fp);
            close(test_fp);
            SP_DEBUG("test end\n");
        } */

        SP_NEW_RETURN(modules[0], ReadFile_Module(1), -1);
        SP_NEW_RETURN(modules[1], PrefixLoad_Module(8), -1);
        SP_NEW_RETURN(modules[2], PrefixMerge_Module(8), -1);

        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }

        fp = fopen(urlPrefixFilterPath, "r");
        // size = sizeoffile(fp);
        size = file_size(urlPrefixFilterPath);
        SP_DEBUG("fp=%p,size=%lld\n", fp, size);
        data->fp_in_ = fp;
        data->length_ = size;
        s_instance_stream->put(msg);
        s_instance_stream->get(msg);
        fclose(fp);
        fp = NULL;
        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->pop();
        }

        SP_DEBUG("url begin\n");

        SP_NEW_RETURN(modules[0], ReadFile_Module(1), -1);
        SP_NEW_RETURN(modules[1], UrlLoad_Module(8), -1);
        SP_NEW_RETURN(modules[2], Write_Module(1), -1);

        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }
        size = file_size(urlidPath);
        fp = fopen(urlidPath, "r");

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
        fclose(fp);
        for (int i = 0; i < data->domain_filter_list_.size(); ++i)
            delete data->domain_filter_list_[i];
        data->domain_filter_list_.clear();
        for (int i = 0; i < data->prefix_filter_list_.size(); ++i)
            delete data->prefix_filter_list_[i];
        data->prefix_filter_list_.clear();
    }
#ifdef LARGE
    else
    {
        //domain file read
        fp = fopen(domainFilterPath, "r");
        data->fp_in_ = fp;
        data->length_ = size;

        SP_NEW_RETURN(s_instance_stream, SP_Stream, -1);
        SP_NEW_RETURN(modules[0], ReadFile_Module(1), -1);
        SP_NEW_RETURN(modules[1], DomainLoad_Module(8), -1);

        for (int i = 1; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }
        SP_NEW_RETURN(msg, SP_Message_Block_Base((SP_Data_Block *)data), -1);
        s_instance_stream->put(msg);
        s_instance_stream->get(msg);
        fclose(fp);
        for (int i = 1; i >= 0; --i)
        {
            s_instance_stream->pop();
        }

        //url file read and domain filter

        SP_NEW_RETURN(modules[0], ReadUrl1_Module(1), -1);
        SP_NEW_RETURN(modules[1], UrlDP_Module(8), -1);
        SP_NEW_RETURN(modules[2], Mid_Module(1), -1);

        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }
        size = file_size(urlidPath);
        //prepare urlfilter
        {
            for (int i = 0; i < 8; ++i)
            {
                UrlFilter *filter = new UrlFilter();
                filter->buf_size_ = FILESPLITSIZE;
                filter->p_ = (char *)malloc(filter->buf_size_ + BUFHEADSIZE);
                filter->p_ = filter->p_ + BUFHEADSIZE;
                // filter->max_list_domainport_count_ = INITURLCOUNT;
                // filter->list_domainport_ = (DomainPortBuf *)malloc(filter->max_list_domainport_count_ * sizeof(DomainPortBuf));
                filter->max_list_count_ = INITURLCOUNT;
                filter->list_ = (char **)malloc(filter->max_list_count_ * sizeof(char *));
                gQueue.push(filter);
            }
        }
        fp = fopen(urlidPath, "r");
        data->fp_in_ = fp;
        data->length_ = size;
        data->fp_out_ = stdout;
        data->reset_para();
        data->mid_file_ = new MidFile();
        data->mid_file_->init();
        s_instance_stream->put(msg);
        s_instance_stream->get(msg);
        {
            for (int i = 0; i < data->domain_filter_list_.size(); ++i)
                delete data->domain_filter_list_[i];
            data->domain_filter_list_.clear();
        }
        fclose(fp);
        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->pop();
        }

        //prefix filter load
        SP_NEW_RETURN(modules[0], ReadFile_Module(1), -1);
        SP_NEW_RETURN(modules[1], PrefixLoad_Module(8), -1);

        for (int i = 1; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }

        fp = fopen(urlPrefixFilterPath, "r");
        // size = sizeoffile(fp);
        size = file_size(urlPrefixFilterPath);
        data->fp_in_ = fp;
        data->length_ = size;
        s_instance_stream->put(msg);
        s_instance_stream->get(msg);
        fclose(fp);
        for (int i = 1; i >= 0; --i)
        {
            s_instance_stream->pop();
        }

        //tmp url read and prefix filter
        SP_NEW_RETURN(modules[0], ReadUrl2_Module(1), -1);
        SP_NEW_RETURN(modules[1], UrlPF_Module(8), -1);
        SP_NEW_RETURN(modules[2], Write_Module(1), -1);

        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }
        data->fp_out_ = stdout;
        data->reset_para();
        s_instance_stream->put(msg);
        s_instance_stream->get(msg);
        dumpCounters(stdout, &data->counter_);
    }
#endif
    return 0;
}
