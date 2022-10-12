#include "util.h"

int main()
{
	int socketfd;
	int res;
	socketfd = socket(AF_INET, SOCK_STREAM, (int)0);
	if ( EXIT_FAIL_CODE == socketfd) {
		printf("%s\n", "socket failed");
		//print_err("socket failed",__LINE__,errno);
	}

	/* set no TIME_WAIT*/
	int reuse = 1;
	res = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(int));
	if (res == EXIT_FAIL_CODE) {
		printf("%s\n", "setsockopt failed");
		close(socketfd);
		_exit(EXIT_FAIL_CODE);
	}


	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(60001);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	res = connect(socketfd,(struct sockaddr*)&addr, sizeof(addr));
	if(EXIT_FAIL_CODE == res) {
		printf("%s: %s\n", "connect failed", strerror(errno));
		//print_err("connect failed", __LINE__, errno);
	}
/*
	char buf[MAX_BUFFER_SIZE] = {0};
	char rec[MAX_BUFFER_SIZE] = {0};
	*/

	while (1) {
		/*
		bzero(&rec, sizeof(recv));
		res = recv(socketfd, &rec, sizeof(rec), 0);
		if(EXIT_FAIL_CODE == res) {
			printf("%s\n", "receive failed");
			//print_err("recv failed", __LINE__, errno);
		}
		if (!strncmp(rec, "CLOSE\n", sizeof("CLOSE\n"))) {
			printf("%s\n", "only support one sender to server");
			printf("%s\n", "receive close signal from server");
			break;
		} else if (!strncmp(rec, "OK\n", sizeof("OK\n"))) {
			printf("%s\n",rec);
		}
		*/


		//bzero(&buf, sizeof(buf));
		//scanf("%s",buf);
		//res = send(socketfd,&buf,sizeof(buf), 0);
		res = send(socketfd,"xyz",sizeof("xyz"), 0);
		if (EXIT_FAIL_CODE == res) {
			printf("%s\n", "send failed");
			//print_err("send failed", __LINE__, errno);
		}
		sleep(1);
	}
	close(socketfd);
	return EXIT_SUCCESS_CODE;
}
