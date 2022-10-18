#include "log.h"
#include "util.h"

TCP_INFO *tcp_info; 
pthread_mutex_t mute;
extern LOG loging;

void tcp_server(threadpool_t *thp) {
	int master_socketfd;
	int res;
	master_socketfd = socket(AF_INET, SOCK_STREAM, (int)0);
	if (EXIT_FAIL_CODE == master_socketfd) {
		LogWrite(ERROR, "%d %s :%s", __LINE__, "master socket failed", strerror(errno));
		_exit(EXIT_FAIL_CODE);
	}
	/* set no TIME_WAIT*/
	int master_reuse = 1;
	res = setsockopt(master_socketfd, SOL_SOCKET, SO_REUSEADDR, (void *)&master_reuse, sizeof(int));
	if (res == EXIT_FAIL_CODE) {
		LogWrite(ERROR, "%d %s :%s", __LINE__, "master setsockopt failed", strerror(errno));
		close(master_socketfd);
		_exit(EXIT_FAIL_CODE);
	}

	LogWrite(DEBUG, "%d %s", __LINE__, "master socket created");

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(tcp_info[0].port);
	addr.sin_addr.s_addr = inet_addr(tcp_info[0].address);

	res = bind(master_socketfd, (struct sockaddr*)&addr, sizeof(addr));
	if (EXIT_FAIL_CODE == res) {
		LogWrite(ERROR, "%d %s :%s", __LINE__, "master bind failed", strerror(errno));
		close(master_socketfd);
		_exit(EXIT_FAIL_CODE);
	}
	LogWrite(DEBUG, "%d %s", __LINE__, "master socket bind");

	res = listen(master_socketfd, MAX_QUEUED_REQUESTS);
	if (EXIT_FAIL_CODE == res) {
		LogWrite(ERROR, "%d %s :%s", __LINE__, "master listen failed", strerror(errno));
		close(master_socketfd);
		_exit(EXIT_FAIL_CODE);
	}
	LogWrite(DEBUG, "%d %s", __LINE__, "master socket listening");

	LogWrite(DEBUG, "%d %s", __LINE__, "master-client socket creating");
	for (int i = 1; i <= tcp_info[0].clientNum; i++) {
		int res = master_client_socket(i);
		if (res == EXIT_FAIL_CODE) {
			LogWrite(ERROR, "%d %s :%s:%d", __LINE__, "master-client socket failed", tcp_info[i].address, tcp_info[i].port);
			tcp_info[i].acceptfd = -1;
		} else {
			LogWrite(DEBUG, "%d %s :%s:%d %d", __LINE__, "master-client socket created", tcp_info[i].address, tcp_info[i].port, tcp_info[i].acceptfd);
		}
	}

	int master_acceptfd = -1;
	while(1) {
		struct sockaddr_in caddr = {0};
		int csize = sizeof(caddr);
		master_acceptfd = accept(master_socketfd, (struct sockaddr*)&caddr, (socklen_t *)&csize);
		if (EXIT_FAIL_CODE == master_acceptfd) {
			LogWrite(ERROR, "%d %s :%s", __LINE__, "master accept failed", strerror(errno));
			_exit(EXIT_FAIL_CODE);
		}
		if (tcp_info[0].acceptfd) {
			LogWrite(ERROR, "%d %s :%d", __LINE__, "master socket fd already occupied by", tcp_info[0].acceptfd);

			/*
			int res;
			res = send(master_acceptfd,"CLOSE\n", sizeof("CLOSE\n"), 0);
			if (EXIT_FAIL_CODE == res) {
				LogWrite(ERROR, "%d %s :%s", __LINE__, "master send failed", strerror(errno));
				break;
			}
			*/
			LogWrite(ERROR, "%d %s", __LINE__, "master only support one sender to server, close connection");
			close(master_acceptfd);
			continue;
		} else {
			/*
			int res;
			res = send(master_acceptfd,"OK\n", sizeof("OK\n"), 0);
			if (EXIT_FAIL_CODE == res) {
				LogWrite(ERROR, "%d %s :%s", __LINE__, "master send failed", strerror(errno));
				break;
			}
			*/
			LogWrite(INFO, "%d %s", __LINE__, "master connection ok");
		}

		tcp_info[0].acceptfd = master_acceptfd;
		LogWrite(DEBUG, "%d %s: %d", __LINE__, "master accept fd get", tcp_info[0].acceptfd);

		LogWrite(INFO, "%d %s%d %s%s", __LINE__, "msater cport=", ntohs(caddr.sin_port), "master caddr=", inet_ntoa(caddr.sin_addr));
		

		// use process
		pid_t pid;
		pid = fork();
		if (pid == EXIT_FAIL_CODE) {
			LogWrite(ERROR, "%d %s :%s", __LINE__, "master fork error", strerror(errno));
			close(tcp_info[0].acceptfd);
			break;
		}

		if (pid == 0) {
			// child process
			LogWrite(DEBUG, "%d %s", __LINE__, "master_client receive process created");
			master_receive(thp);
		}
	}
}

int master_client_socket(int index) {
	int socketfd;
	int res;
	socketfd = socket(AF_INET, SOCK_STREAM, (int)0);
	if ( EXIT_FAIL_CODE == socketfd) {
		LogWrite(ERROR, "%d %s :%s", __LINE__, "master-client socket failed", strerror(errno));
		return EXIT_FAIL_CODE;
	}

	/* set no TIME_WAIT*/
	int reuse = 1;
	res = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(int));
	if (res == EXIT_FAIL_CODE) {
		LogWrite(ERROR, "%d %s :%s", __LINE__, "master-client setsockopt failed", strerror(errno));
		close(socketfd);
		return EXIT_FAIL_CODE;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(tcp_info[index].port);
	addr.sin_addr.s_addr = inet_addr(tcp_info[index].address);

	res = connect(socketfd,(struct sockaddr*)&addr, sizeof(addr));
	if(EXIT_FAIL_CODE == res) {
		LogWrite(ERROR, "%d %s :%s", __LINE__, "master-client connect failed", tcp_info[index].address);
		return EXIT_FAIL_CODE;
	}
	tcp_info[index].acceptfd = socketfd;
	return EXIT_SUCCESS_CODE;
}

void master_receive(threadpool_t *thp) {
	int master_acceptfd;
	int client_number = tcp_info[0].clientNum;
	pthread_t tids[client_number];
	char buf[MAX_BUFFER_SIZE] = {0};
	master_acceptfd = tcp_info[0].acceptfd;

	THREAD_PARAM thread_param;

	while(1) {
		int res;
		bzero(&buf, sizeof(buf));
		res = recv(master_acceptfd, &buf, sizeof(buf), (int)0);

		if (EXIT_FAIL_CODE == res) {
			LogWrite(ERROR, "%d %s :%s", __LINE__, "master receive failed", strerror(errno));
			break;
		}
		else if (res > 0) {
			/*直接读buf会碰到0结束的情况*/
			LogWrite(INFO, "%d %s:", __LINE__, "master received message");
			for (int i = 0; i < res; i++) {
				fprintf(loging.logfile, "%#2X", buf[i]);
			}
			LogWrite(DEBUG, "\n");
		} else if (res == 0) {
			break;
		}
 
/*
// convert
		int8_t temp[4096] = {0};
		int len;
		AsciiToHex(buf, temp, strlen(buf));
		if (strlen(buf)%2 > 0) {
        	len = strlen(buf)/2 + 1;
    	} else {
        	len = strlen(buf)/2;
    	}

    	for( int i = 0; i < len; i++) {
        	printf("int[%d] is %X,%d\n", i+1,temp[i],temp[i]);
    	}
*/		
		// strncpy在拷贝的时候，即使长度还没到，但是遇到0也会自动截断
		//strncpy(thread_param.buf, buf, sizeof(buf));
		memcpy(thread_param.buf, buf, res);
		thread_param.bufSize = res;
		pthread_mutex_init(&mute, NULL);
		LogWrite(DEBUG, "%d %s", __LINE__, "thread mutex init");
		for (int i = 0; i < client_number; i++) {
			//创建线程锁
			thread_param.clientIndex = i + 1;
			int ret = pthread_create(&tids[i], NULL, master_client_send, (void *)&thread_param);
			if (EXIT_FAIL_CODE == ret) {
				LogWrite(ERROR, "%d %s :%s", __LINE__, "master-client thread create failed", strerror(errno));
				close(tcp_info[i+1].acceptfd);
				tcp_info[i+1].acceptfd = 0;
			}
			for (int i = 0; i < client_number; i++) {
				pthread_join(tids[i], NULL);
			}

		}
		//释放锁
		LogWrite(DEBUG, "%d %s", __LINE__, "thread mutex destroy");
		pthread_mutex_destroy(&mute);
		bzero(&thread_param.buf, sizeof(thread_param.buf));


/*
		res = send(master_acceptfd,"OK\n", sizeof("OK\n"), 0);
		if (EXIT_FAIL_CODE == res) {
			LogWrite(ERROR, "%d %s :%s", __LINE__, "send failed", strerror(errno));
			break;
		}
		
	*/
	}
	LogWrite(INFO, "%d %s", __LINE__, "receive done");
	tcp_info[0].acceptfd = 0;
	close(master_acceptfd);
}

void *master_client_send(void *pth_arg) {
	pthread_mutex_lock(&mute);
	THREAD_PARAM *thread_param = (THREAD_PARAM *)pth_arg;
	LogWrite(DEBUG, "%d %s:%d", __LINE__, "thread locked by client", thread_param->clientIndex);
	char *buf = thread_param->buf;
	int index = thread_param->clientIndex;
	int bufSize = thread_param->bufSize;

	printf("thread_param->clientIndex: %d\n", index);
	LogWrite(DEBUG, "%d %s :%d", __LINE__, "master-client thread created and acceptfd", tcp_info[index].acceptfd);

	int res = send(tcp_info[index].acceptfd, buf, bufSize, 0);
	if (EXIT_FAIL_CODE == res) {
		LogWrite(ERROR, "%d %s %s :%s:%d", __LINE__, "send failed", strerror(errno), tcp_info[index].address, tcp_info[index].port);
	} else if(res > 0) {
		LogWrite(DEBUG, "%d %s %s:%d", __LINE__, "send msgs to", tcp_info[index].address, tcp_info[index].port);
	}
	LogWrite(DEBUG, "%d %s:%d", __LINE__, "thread unlocked by client", thread_param->clientIndex);
	pthread_mutex_unlock(&mute);

	return pth_arg;
}

void parseFile_tcpInfo(TCP_INFO **tcp_info) {
	char path[512] = {0x0};
	char **fileData = NULL;
	int lines = 0;
	int clientNum = 0;

    getcwd(path, sizeof(path));
    strcat(path, CONFIGFILE);

	struct ConfigInfo *info = NULL;
	//加载配置文件
	loadFile_ConfigFile(path, &fileData, &lines);
	//解析配置文件
	parseFile_ConfigFile(fileData, lines, &info);

	clientNum = atoi(getInfo_ConfigFile("client-num", info, lines));

	LogWrite(INFO, "%d %s :%d", __LINE__, "client number is", clientNum);

	TCP_INFO *ptcp_info = (TCP_INFO *)malloc(sizeof(TCP_INFO) * (clientNum + 1));
	memset(ptcp_info, 0, sizeof(TCP_INFO) * (clientNum + 1));

	//TCP_INFO tcp_info[clientNum + 1];
	TCP_INFO master = {0x0};
	strcpy(master.address, getInfo_ConfigFile("master-address", info, lines));
	master.port = atoi(getInfo_ConfigFile("master-port", info, lines));
	master.clientNum = clientNum;
	ptcp_info[0] = master; //存的非指针,所以不存在stack释放引用错误的问题
	LogWrite(DEBUG, "%d %s :%s", __LINE__, "master address is", ptcp_info[0].address);
	LogWrite(DEBUG, "%d %s :%d", __LINE__, "master port is", ptcp_info[0].port);
	LogWrite(DEBUG, "%d %s :%d", __LINE__, "master connected clients number are", ptcp_info[0].clientNum);
	for(int i = 1; i <= clientNum; i++) {
		// create client struct
		TCP_INFO client = {0x0};
		
		char baseName[20]={0x0};
		char baseAddressName[20]={0x0};
		char basePortName[20]={0x0};
		char charValue[5];

		strncpy(baseName, "client", 6);
		sprintf(charValue, "%d", i);
		strcat(baseName, charValue);

		strcpy(baseAddressName, baseName);
		strcat(baseAddressName, "-address");
		strcpy(basePortName, baseName);
		strcat(basePortName, "-port");

		strcpy(client.address, getInfo_ConfigFile(baseAddressName, info, lines));
		client.port = atoi(getInfo_ConfigFile(basePortName, info, lines));

		ptcp_info[i] = client;

		LogWrite(DEBUG, "%d %s %s :%s", __LINE__, baseName, "address is", ptcp_info[i].address);
		LogWrite(DEBUG, "%d %s %s :%d", __LINE__, baseName, "port is", ptcp_info[i].port);
	}

	*tcp_info = ptcp_info;
}