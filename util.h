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

#define INFO_LENGTH                 128
#define BUFFER_SIZE                 512

// N connection requests will be queued before further requests are refused.
#define MAX_QUEUED_REQUESTS         5

// receive/send buffer
#define MAX_BUFFER_SIZE             4096


typedef struct thread_param {
	char				buf[MAX_BUFFER_SIZE];
	int					bufSize;
	int					clientIndex;
}THREAD_PARAM;

typedef struct tcpInfo {
	char            	address[INFO_LENGTH];
	int            		port;
	int					acceptfd;
	int					clientNum;
}TCP_INFO;

//void tcp_server(TCP_INFO *tcp_info);
void tcp_server(threadpool_t *thp);
void parseFile_tcpInfo(TCP_INFO **tcp_info);
void *master_client_send(void *pth_arg);
//void master_receive(TCP_INFO *tcp_info);
void master_receive(threadpool_t *thp);
int master_client_socket(int index);