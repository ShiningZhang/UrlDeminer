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

using namespace std;

static SP_Stream *s_instance_stream = NULL;
static SP_Module *modules[10];

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
    FilterCounters counters;
    const char *domainFilterPath = argv[1];
    const char *urlPrefixFilterPath = argv[2];
    const char *urlidPath = argv[3];
    const char *urlfeature = argv[4];

    Request *data = new Request();
    SP_Message_Block_Base *msg = NULL;

    uint64_t size = file_size(domainFilterPath);
    FILE *fp;
    if (size < SMALLSIZE)
    {
        fp = fopen(domainFilterPath, "r");
        // uint64_t size = sizeoffile(fp);
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

        SP_NEW_RETURN(modules[0], ReadFile_Module(1), -1);
        SP_NEW_RETURN(modules[1], UrlLoad_Module(8), -1);
        SP_NEW_RETURN(modules[2], Write_Module(1), -1);

        for (int i = 2; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }
        size = file_size(urlidPath);
        fp = fopen(urlidPath, "r");
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
    else
    {
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

    }

    return 0;
}
