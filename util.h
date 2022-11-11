#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include "log.h"
#include "threadpool.h"
#include "recordData.h"

#define INFO_LENGTH                 128
#define BUFFER_SIZE                 512

// N connection requests will be queued before further requests are refused.
#define MAX_QUEUED_REQUESTS         5

// receive/send buffer
#define MAX_BUFFER_SIZE             4096000

#define PLAYBACK                    2
#define DISTRIBUTE                  1


typedef struct thread_param {
	unsigned char		buf[MAX_BUFFER_SIZE];
	int					bufSize;
	int					clientIndex;
}THREAD_PARAM;

typedef struct tcpInfo {
	char            	address[INFO_LENGTH]; // self addresses
	char            	acceptedAddress[INFO_LENGTH];
	int            		port;
	int					acceptfd;
	int					clientNum;
    int                 flag;
}DISTRIBUTE_TCP_INFO;

void server_start(threadpool_t *thp);
void distribute_server(threadpool_t *thp);
void distribute_receive(threadpool_t *thp);
void distribute_run(unsigned char *receive_buf, int receive_size);
void *distribute_client_send(void *pth_arg);
void parseFile_distributeTcpInfo(DISTRIBUTE_TCP_INFO **distributeTcpInfo);
