#define _CRT_SECURE_NO_WARNINGS
#include "util.h"
#include "ConfigFile.h"
DISTRIBUTE_TCP_INFO *distributeTcpInfo;
int main(int argc, char ** argv)
{
	LogWrite(INFO, "%d %s", __LINE__, "*********Start Server*********");


	LogWrite(INFO, "%d %s", __LINE__, "parse config file to distribute&playbook");
	
	extern DISTRIBUTE_TCP_INFO *distributeTcpInfo;
	parseFile_distributeTcpInfo(&distributeTcpInfo);


threadpool_t thp;
/*
	LogWrite(INFO, "%d %s", __LINE__, "init thread pool");
	threadpool_t *thp = threadpool_create(MIN_THREAD_NUM, MAX_THREAD_NUM, QUEUE_MAX_SIZE);
	if (thp == NULL) {
		LogWrite(ERROR, "%d %s", __LINE__, "thread pool init error");
		return EXIT_FAIL_CODE;
	}
*/
	LogWrite(INFO, "%d %s", __LINE__, "init tcp connection");

    server_start(&thp);


	return EXIT_SUCCESS_CODE;
}