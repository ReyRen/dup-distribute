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

        distributeTcpInfo[0].acceptfd = distribute_acceptfd;
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
			close(distributeTcpInfo[0].acceptfd);
			break;
		}

		if (pid == 0) {
			// child process
			LogWrite(DEBUG, "%d %s", __LINE__,
                     "distribute-client receive process created");

            /*每个进程都有独立的内存空间，所以不用担心distributeTcpInfo[0]被覆盖*/
			distribute_receive(thp);
		}
	}
}

int distribute_client_socket(int index) {
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
	addr.sin_port = htons(distributeTcpInfo[index].port);
	addr.sin_addr.s_addr = inet_addr(distributeTcpInfo[index].address);

	res = connect(socketfd,(struct sockaddr*)&addr, sizeof(addr));
	if(EXIT_FAIL_CODE == res) {
		LogWrite(ERROR, "%d %s :%s", __LINE__,
                 "master-client connect failed", distributeTcpInfo[index].address);
        close(socketfd);
		return EXIT_FAIL_CODE;
	}
    distributeTcpInfo[index].acceptfd = socketfd;
	return socketfd;
}

/*
 * accept后的处理进程收到信息先处理一下头
 * 回放请求：不进行回放请求数据记录
 *           ->创建多client线程
 *             ->不同client进行连接创建
 *               ->不同client线程判断回放请求的地址是不是和自己创建的地址一样
 *                 ->和自己地址一致的执行playback逻辑
 *                   ->创建带地址的共享文件让别的进程知道这个client正在进行回放，停止转发
 *                 ->和自己地址不一致的退出该线程
 * 海事/北斗：进行转发数据记录
 *            ->创建多client线程
 *              ->不同client进行连接创建
 *              ->不同client进行判断是否有和自己一致的共享文件，如果有则停止这次转发
 * */
void distribute_receive(threadpool_t *thp) {

	unsigned char receive_buf[MAX_BUFFER_SIZE] = {0}; // buffer 4M防止粘包处理

    SeaCommunication seaCommunication;
    BDCommunication bdCommunication;
    ReplayProtocol replayProtocol;
    bzero(&seaCommunication, sizeof(SeaCommunication));
    bzero(&bdCommunication, sizeof(BDCommunication));
    bzero(&replayProtocol, sizeof(ReplayProtocol));

	while(1) {
		int receive_size;
		bzero(&receive_buf, sizeof(receive_buf));
        receive_size = recv(distributeTcpInfo[0].acceptfd, &receive_buf, sizeof(receive_buf), (int)0);

		if (EXIT_FAIL_CODE == receive_size) {
			LogWrite(ERROR, "%d %s :%s", __LINE__,
                     "distribute receive failed", strerror(errno));
			break;
		}
        else if (receive_size > 0) {
            LogWrite(INFO, "%d %s", __LINE__, "distribute received message");

            memcpy(&seaCommunication, receive_buf, sizeof(SeaCommunication));
            memcpy(&bdCommunication, receive_buf, sizeof(BDCommunication));
            memcpy(&replayProtocol, receive_buf, sizeof(ReplayProtocol));

            if (seaCommunication.PacketHead == SEACOMMHEADER ||
                    bdCommunication.PacketHead == BDCOMMHEADER) {
                distributeTcpInfo[0].flag = DISTRIBUTE;
                distribute_run(receive_buf, receive_size);
            } else if (replayProtocol.PacketHead == PLAYBACKHEADER){
                LogWrite(INFO, "%d %s", __LINE__,
                         "distribute received playback signal, not record");
                distributeTcpInfo[0].flag = PLAYBACK;
                distribute_run(receive_buf, receive_size);
            } else {
                LogWrite(INFO, "%d %s", __LINE__,
                         "distribute received useless message, discard");
            }
		} else if (receive_size == 0) {
			break;
		}
	}
	LogWrite(INFO, "%d %s", __LINE__, "distribute all done");
	close(distributeTcpInfo[0].acceptfd);
}

/*
 * 处理文件记录以及创建client线程
 * */
void distribute_run(unsigned char *receive_buf,
                    int receive_size) {
    FILE *file = {0x0};
    pthread_t tids[distributeTcpInfo[0].clientNum];
    THREAD_PARAM thread_param;

    bzero(&thread_param, sizeof(THREAD_PARAM));
    /*
     * 直接读buf会碰到0结束的情况
        发送方给的数据就是一个字节的16进制数（0x89这类型），一16进制是4bit，也就是半字节。所以定义接收
        16进制数，主要得知道接收的每个16进制数的大小。
        char就是一个字节，unsigned char可以将打印出的16进的fff解决（是因为char是有符号的，16进制转换2进制头是1的话就会有fff）
    */
    if (distributeTcpInfo[0].flag == DISTRIBUTE) {
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
    for (int i = 0; i < distributeTcpInfo[0].clientNum; i++) {
        //创建线程锁
        pthread_mutex_lock(&mute);
        thread_param.clientIndex = i + 1;
        int ret = pthread_create(&tids[i], NULL, distribute_client_send, (void *)&thread_param);
        if (EXIT_FAIL_CODE == ret) {
            LogWrite(ERROR, "%d %s :%s", __LINE__,
                     "distribute-client thread create failed", strerror(errno));
            close(distributeTcpInfo[0].acceptfd);
        }
    }
    for (int i = 0; i < distributeTcpInfo[0].clientNum; i++) {
        pthread_join(tids[i], NULL);
    }
    //释放锁
    LogWrite(DEBUG, "%d %s", __LINE__, "distribute-client thread mutex destroy");
    pthread_mutex_destroy(&mute);
    bzero(&thread_param.buf, sizeof(thread_param.buf));
}

void *distribute_client_send(void *pth_arg) {
    THREAD_PARAM *thread_param = (THREAD_PARAM *)pth_arg;
    LogWrite(DEBUG, "%d %s:%d", __LINE__,
             "thread locked by client", thread_param->clientIndex);

    int socketfd;

    LogWrite(DEBUG, "%d %s", __LINE__, "distribute client socket creating");
    socketfd = distribute_client_socket(thread_param->clientIndex); // -1 error
    if (socketfd == EXIT_FAIL_CODE) {
        LogWrite(ERROR, "%d %s :%s:%d", __LINE__,
                 "distribute-client socket failed, only record",
                 distributeTcpInfo[thread_param->clientIndex].address,
                 distributeTcpInfo[thread_param->clientIndex].port);
    } else {
        LogWrite(DEBUG, "%d %s :%s:%d %d", __LINE__,
                 "distribute-client socket created",
                 distributeTcpInfo[thread_param->clientIndex].address,
                 distributeTcpInfo[thread_param->clientIndex].port, socketfd);
        LogWrite(DEBUG, "%d %s :%d", __LINE__,
                 "distribute-client thread created and acceptfd", socketfd);
        ReplayProtocol replayProtocol;
        bzero(&replayProtocol, sizeof(ReplayProtocol));
        memcpy(&replayProtocol, thread_param->buf, sizeof(ReplayProtocol));
        /*当收到的数据是回放请求头，并且当前client线程地址和请求的地址一样时，执行回放*/
        if (replayProtocol.PacketHead == PLAYBACKHEADER &&
                strcmp(distributeTcpInfo[thread_param->clientIndex].address,
                       distributeTcpInfo[0].address)){
            LogWrite(INFO, "%d %s", __LINE__,
                     "distribute-client accepted, and get playback signal, start execute playback");
            playback_run(thread_param->buf,
                         thread_param->bufSize,
                         thread_param->clientIndex);
        }

        /*判断共享文件是否存在*/
        char shmFile[128] = {0x0};
        strcat(shmFile, "./shm/");
        strcat(shmFile, distributeTcpInfo[thread_param->clientIndex].address);

        if (access(shmFile, F_OK) != 0) {
            // 文件存在，不进行转发
            int res = send(socketfd, thread_param->buf, thread_param->bufSize, 0);
            if (EXIT_FAIL_CODE == res) {
                LogWrite(ERROR, "%d %s %s :%s:%d", __LINE__, "send [FAIL] to",
                         strerror(errno),
                         distributeTcpInfo[thread_param->clientIndex].address,
                         distributeTcpInfo[thread_param->clientIndex].port);
            } else if(res > 0) {
                LogWrite(DEBUG, "%d %s %s:%d", __LINE__, "send [SUCCESS] to",
                         distributeTcpInfo[thread_param->clientIndex].address,
                         distributeTcpInfo[thread_param->clientIndex].port);
            }
        }
        close(socketfd);
    }

	LogWrite(DEBUG, "%d %s:%d", __LINE__,
             "thread unlocked by client", thread_param->clientIndex);
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

    DISTRIBUTE_TCP_INFO *pdistributeTcpInfo =
            (DISTRIBUTE_TCP_INFO *)malloc(sizeof(DISTRIBUTE_TCP_INFO) * (clientNum + 1));
	memset(pdistributeTcpInfo, 0, sizeof(DISTRIBUTE_TCP_INFO) * (clientNum + 1));

	//TCP_INFO tcp_info[clientNum + 1];
    DISTRIBUTE_TCP_INFO master = {0x0};
	strcpy(master.address, getInfo_ConfigFile("distribute-address", info, lines));
	master.port = atoi(getInfo_ConfigFile("distribute-port", info, lines));
	master.clientNum = clientNum;
    pdistributeTcpInfo[0] = master; //存的非指针,所以不存在stack释放引用错误的问题
	LogWrite(DEBUG, "%d %s :%s", __LINE__,
             "distribute address is", pdistributeTcpInfo[0].address);
	LogWrite(DEBUG, "%d %s :%d", __LINE__,
             "distribute port is", pdistributeTcpInfo[0].port);
	LogWrite(DEBUG, "%d %s :%d", __LINE__,
             "distribute connected clients number are", pdistributeTcpInfo[0].clientNum);
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