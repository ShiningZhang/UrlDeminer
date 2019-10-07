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
#include "util.h"

using namespace std;



GlobalST g_;

int main(int argc, char **argv)
{
	FilterCounters counters;
	if (argc >= 4)
	{
		const char *domainFilterPath = argv[1];
		const char *urlPrefixFilterPath = argv[2];
		const char *urlidPath = argv[3];
		const char *urlfeature = argv[4];
		bool ret = false;
		ret = DomainFilter::load(domainFilterPath);
	}
	return 0;
}