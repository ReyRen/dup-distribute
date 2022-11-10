#include "util.h"
#include "playback.h"

DISTRIBUTE_TCP_INFO *distributeTcpInfo;
unsigned int uuid;
pthread_mutex_t mute;

void server_start(threadpool_t *thp) {
    return distribute_server(thp);
}

void distribute_server(threadpool_t *thp) {
	int distribute_socketfd;
	int res;
    distribute_socketfd = socket(AF_INET, SOCK_STREAM, (int)0);
	if (EXIT_FAIL_CODE == distribute_socketfd) {
		LogWrite(ERROR, "%d %s :%s", __LINE__,
                 "distribute socket failed", strerror(errno));
		_exit(EXIT_FAIL_CODE);
	}
	/* set no TIME_WAIT*/
	int distribute_reuse = 1;
	res = setsockopt(distribute_socketfd, SOL_SOCKET, SO_REUSEADDR, (void *)&distribute_reuse, sizeof(int));
	if (res == EXIT_FAIL_CODE) {
		LogWrite(ERROR, "%d %s :%s", __LINE__,
                 "distribute setsockopt failed", strerror(errno));
		close(distribute_socketfd);
		_exit(EXIT_FAIL_CODE);
	}

	LogWrite(DEBUG, "%d %s", __LINE__, "distribute socket created");

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(distributeTcpInfo[0].port);
	addr.sin_addr.s_addr = inet_addr(distributeTcpInfo[0].address);

	res = bind(distribute_socketfd, (struct sockaddr*)&addr, sizeof(addr));
	if (EXIT_FAIL_CODE == res) {
		LogWrite(ERROR, "%d %s :%s", __LINE__,
                 "distribute bind failed", strerror(errno));
		close(distribute_socketfd);
		_exit(EXIT_FAIL_CODE);
	}
	LogWrite(DEBUG, "%d %s", __LINE__, "distribute socket bind");

	res = listen(distribute_socketfd, MAX_QUEUED_REQUESTS);
	if (EXIT_FAIL_CODE == res) {
		LogWrite(ERROR, "%d %s :%s", __LINE__,
                 "distribute listen failed", strerror(errno));
		close(distribute_socketfd);
		_exit(EXIT_FAIL_CODE);
	}
	LogWrite(DEBUG, "%d %s", __LINE__, "distribute socket listening");

	int distribute_acceptfd = -1;
	while(1) {
		struct sockaddr_in caddr = {0};
		int csize = sizeof(caddr);

        //从这里开始，全局distributeTcpInfo就尽量不要使用了
        distribute_acceptfd = accept(distribute_socketfd, (struct sockaddr*)&caddr, (socklen_t *)&csize);
		if (EXIT_FAIL_CODE == distribute_acceptfd) {
			LogWrite(ERROR, "%d %s :%s", __LINE__, "distribute accept failed", strerror(errno));
			_exit(EXIT_FAIL_CODE);
		}

        LogWrite(INFO, "%d %s", __LINE__, "distribute connection ok");

		LogWrite(DEBUG, "%d %s: %d", __LINE__,
                 "distribute accept fd get", distribute_acceptfd);

		LogWrite(INFO, "%d %s%d %s%s", __LINE__,
                 "distribute cport=", ntohs(caddr.sin_port), "distribute caddr=", inet_ntoa(caddr.sin_addr));

		// use process
		pid_t pid;
		pid = fork();
		if (pid == EXIT_FAIL_CODE) {
			LogWrite(ERROR, "%d %s :%s", __LINE__,
                     "distribute fork error", strerror(errno));
			close(distribute_acceptfd);
			break;
		}

		if (pid == 0) {
			// child process
			LogWrite(DEBUG, "%d %s", __LINE__,
                     "distribute-client receive process created");
			distribute_receive(thp, distribute_acceptfd);
		}
	}
}

int distribute_client_socket(int index, int port, char *address) {
	int socketfd;
	int res;
	socketfd = socket(AF_INET, SOCK_STREAM, (int)0);
	if ( EXIT_FAIL_CODE == socketfd) {
		LogWrite(ERROR, "%d %s :%s", __LINE__,
                 "master-client socket failed", strerror(errno));
		return EXIT_FAIL_CODE;
	}

	/* set no TIME_WAIT*/
	int reuse = 1;
	res = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(int));
	if (res == EXIT_FAIL_CODE) {
		LogWrite(ERROR, "%d %s :%s", __LINE__,
                 "distribute-client setsockopt failed", strerror(errno));
		close(socketfd);
		return EXIT_FAIL_CODE;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(address);

	res = connect(socketfd,(struct sockaddr*)&addr, sizeof(addr));
	if(EXIT_FAIL_CODE == res) {
		LogWrite(ERROR, "%d %s :%s", __LINE__, "master-client connect failed", address);
        close(socketfd);
		return EXIT_FAIL_CODE;
	}
    distributeTcpInfo[index].acceptfd = socketfd;
	return socketfd;
}

void distribute_receive(threadpool_t *thp, int distribute_acceptfd) {

	unsigned char buf[MAX_BUFFER_SIZE] = {0};

    SeaCommunication seaCommunication;
    BDCommunication bdCommunication;
    ReplayProtocol replayProtocol;
    bzero(&seaCommunication, sizeof(SeaCommunication));
    bzero(&bdCommunication, sizeof(BDCommunication));
    bzero(&replayProtocol, sizeof(ReplayProtocol));

	while(1) {
		int res;
		bzero(&buf, sizeof(buf));
		res = recv(distribute_acceptfd, &buf, sizeof(buf), (int)0);

		if (EXIT_FAIL_CODE == res) {
			LogWrite(ERROR, "%d %s :%s", __LINE__, "distribute receive failed", strerror(errno));
			break;
		}
        else if (res > 0) {
            LogWrite(INFO, "%d %s", __LINE__, "distribute received message");

            memcpy(&seaCommunication, buf, sizeof(SeaCommunication));
            memcpy(&bdCommunication, buf, sizeof(BDCommunication));
            memcpy(&replayProtocol, buf, sizeof(ReplayProtocol));

            if (seaCommunication.PacketHead == SEACOMMHEADER ||
                    bdCommunication.PacketHead == BDCOMMHEADER) {
                distribute_run(buf, res, distribute_acceptfd, 1);
            } else if (replayProtocol.PacketHead == PLAYBACKHEADER){
                LogWrite(INFO, "%d %s", __LINE__, "distribute received playback signal, not record");
                distribute_run(buf, res, distribute_acceptfd, 0);
            } else {
                LogWrite(INFO, "%d %s", __LINE__, "distribute received useless message, discard");
            }
		} else if (res == 0) {
			break;
		}
	}
	LogWrite(INFO, "%d %s", __LINE__, "distribute all done");
    //distributeTcpInfo[0].acceptfd = 0;
	close(distribute_acceptfd);
}

void distribute_run(unsigned char *receive_buf,
                    int receive_size,
                    int distribute_acceptfd,
                    int recordFD) {
    FILE *file = {0x0};
    int client_number = distributeTcpInfo[0].clientNum;
    pthread_t tids[client_number];
    THREAD_PARAM thread_param;

    bzero(&thread_param, sizeof(THREAD_PARAM));
    /*
     * 直接读buf会碰到0结束的情况
        发送方给的数据就是一个字节的16进制数（0x89这类型），一16进制是4bit，也就是半字节。所以定义接收
        16进制数，主要得知道接收的每个16进制数的大小。
        char就是一个字节，unsigned char可以将打印出的16进的fff解决（是因为char是有符号的，16进制转换2进制头是1的话就会有fff）
    */
    if (recordFD) {
        file = initDataRecord(file);
        if (file == NULL) {
            LogWrite(ERROR, "%d %s:", __LINE__, "distribute initDataRecord failed");
            return;
        }
        /*
            记录传输数据
        */
        fwrite(receive_buf, sizeof(char), receive_size, file);
        fclose(file);
    }
    // strncpy在拷贝的时候，即使长度还没到，但是遇到0也会自动截断
    //strncpy(thread_param.buf, buf, sizeof(buf));
    memcpy(thread_param.buf, receive_buf, receive_size);
    thread_param.bufSize = receive_size;
    pthread_mutex_init(&mute, NULL);
    LogWrite(DEBUG, "%d %s", __LINE__, "distribute-client thread mutex init");
    for (int i = 0; i < client_number; i++) {
        //创建线程锁
        pthread_mutex_lock(&mute);
        thread_param.clientIndex = i + 1;
        int ret = pthread_create(&tids[i], NULL, distribute_client_send, (void *)&thread_param);
        if (EXIT_FAIL_CODE == ret) {
            LogWrite(ERROR, "%d %s :%s", __LINE__,
                     "distribute-client thread create failed", strerror(errno));
            close(distribute_acceptfd);
        }
    }
    for (int i = 0; i < client_number; i++) {
        pthread_join(tids[i], NULL);
    }
    //释放锁
    LogWrite(DEBUG, "%d %s", __LINE__, "distribute-client thread mutex destroy");
    pthread_mutex_destroy(&mute);
    bzero(&thread_param.buf, sizeof(thread_param.buf));
}

void *distribute_client_send(void *pth_arg) {
    THREAD_PARAM *thread_param = (THREAD_PARAM *)pth_arg;
    LogWrite(DEBUG, "%d %s:%d", __LINE__, "thread locked by client", thread_param->clientIndex);
    unsigned char *buf = thread_param->buf;
    int index = thread_param->clientIndex;
    int bufSize = thread_param->bufSize;
    //永远不要信任共享的全局变量!!!!!!!!
    int port = distributeTcpInfo[index].port;
    char addressBuf[50];
    bzero(addressBuf, sizeof addressBuf);
    strcpy(addressBuf, distributeTcpInfo[index].address);

    int socketfd;

    LogWrite(DEBUG, "%d %s", __LINE__, "distribute client socket creating");
    socketfd = distribute_client_socket(index, port, addressBuf); // -1 error
    if (socketfd == EXIT_FAIL_CODE) {
        LogWrite(ERROR, "%d %s :%s:%d", __LINE__,
                 "distribute-client socket failed, only record",
                 addressBuf, port);
    } else {
        //distributeTcpInfo[index].acceptfd = socketfd;
        LogWrite(DEBUG, "%d %s :%s:%d %d", __LINE__,
                 "distribute-client socket created",
                 addressBuf, port, socketfd);
        LogWrite(DEBUG, "%d %s :%d", __LINE__,
                 "distribute-client thread created and acceptfd", socketfd);
        ReplayProtocol replayProtocol;
        bzero(&replayProtocol, sizeof(ReplayProtocol));
        memcpy(&replayProtocol, buf, sizeof(ReplayProtocol));
        if (replayProtocol.PacketHead == PLAYBACKHEADER){
            distributeTcpInfo[index].playbackFlag = 1;
            LogWrite(INFO, "%d %s", __LINE__,
                     "distribute-client accepted, and get playback signal, start execute playback");
            playback_run(buf, bufSize, index);
        }
        int playbackFlag = distributeTcpInfo[index].playbackFlag;
        printf("ppppppppppppppppppppppppppppppppppppppppp: %d\n", distributeTcpInfo[index].playbackFlag);
        if (!distributeTcpInfo[index].playbackFlag) {
            int res = send(socketfd, buf, bufSize, 0);
            if (EXIT_FAIL_CODE == res) {
                LogWrite(ERROR, "%d %s %s :%s:%d", __LINE__, "send [FAIL] to",
                         strerror(errno), addressBuf, port);
            } else if(res > 0) {
                LogWrite(DEBUG, "%d %s %s:%d", __LINE__, "send [SUCCESS] to", addressBuf, port);
            }
            close(socketfd);
        } else {
            LogWrite(INFO, "%d %s", __LINE__,
                     "playback mode, not distribute-client, only record!");
        }
    }

	LogWrite(DEBUG, "%d %s:%d", __LINE__, "thread unlocked by client", index);
	pthread_mutex_unlock(&mute);

	return pth_arg;
}

void parseFile_distributeTcpInfo(DISTRIBUTE_TCP_INFO **distributeTcpInfo) {
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

    DISTRIBUTE_TCP_INFO *pdistributeTcpInfo = (DISTRIBUTE_TCP_INFO *)malloc(sizeof(DISTRIBUTE_TCP_INFO) * (clientNum + 1));
	memset(pdistributeTcpInfo, 0, sizeof(DISTRIBUTE_TCP_INFO) * (clientNum + 1));

	//TCP_INFO tcp_info[clientNum + 1];
    DISTRIBUTE_TCP_INFO master = {0x0};
	strcpy(master.address, getInfo_ConfigFile("distribute-address", info, lines));
	master.port = atoi(getInfo_ConfigFile("distribute-port", info, lines));
	master.clientNum = clientNum;
    pdistributeTcpInfo[0] = master; //存的非指针,所以不存在stack释放引用错误的问题
	LogWrite(DEBUG, "%d %s :%s", __LINE__, "distribute address is", pdistributeTcpInfo[0].address);
	LogWrite(DEBUG, "%d %s :%d", __LINE__, "distribute port is", pdistributeTcpInfo[0].port);
	LogWrite(DEBUG, "%d %s :%d", __LINE__, "distribute connected clients number are", pdistributeTcpInfo[0].clientNum);
	for(int i = 1; i <= clientNum; i++) {
		// create client struct
        DISTRIBUTE_TCP_INFO client = {0x0};
		
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

        pdistributeTcpInfo[i] = client;

		LogWrite(DEBUG, "%d %s %s :%s", __LINE__, baseName, "address is", pdistributeTcpInfo[i].address);
		LogWrite(DEBUG, "%d %s %s :%d", __LINE__, baseName, "port is", pdistributeTcpInfo[i].port);
	}

	*distributeTcpInfo = pdistributeTcpInfo;
}