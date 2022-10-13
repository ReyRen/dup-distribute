#include "util.h"

int main(int argc, char **argv) {
    int socketfd;
	int res;
	socketfd = socket(AF_INET, SOCK_STREAM, (int)0);
	if (EXIT_FAIL_CODE == socketfd) {
        printf("%s\n", "socket failed");
		_exit(EXIT_FAIL_CODE);
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
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_addr.s_addr = inet_addr(argv[2]);

	res = bind(socketfd, (struct sockaddr*)&addr, sizeof(addr));
	if (EXIT_FAIL_CODE == res) {
        printf("%s\n", "bind failed");
		close(socketfd);
		_exit(EXIT_FAIL_CODE);
	}
    printf("%s\n", "bind done");

	res = listen(socketfd, MAX_QUEUED_REQUESTS);
	if (EXIT_FAIL_CODE == res) {
        printf("%s\n", "listen failed");
		close(socketfd);
		_exit(EXIT_FAIL_CODE);
	}
    printf("%s\n", "listen done");

    int acceptfd = -1;
    struct sockaddr_in caddr = {0};
	int csize = sizeof(caddr);
	acceptfd = accept(socketfd, (struct sockaddr*)&caddr, (socklen_t *)&csize);
    char buf[MAX_BUFFER_SIZE] = {0};
    while (1)
    {
		bzero(&buf, sizeof(buf));
		int res = recv(acceptfd, &buf, sizeof(buf), (int)0);

		if (EXIT_FAIL_CODE == res) {
            printf("%s\n", "receive done");
			break;
		}
		else if (res > 0) {
		}
    }
    
}