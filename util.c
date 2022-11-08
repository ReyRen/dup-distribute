#include "util.h"
#include "dataHeader.h"

DISTRIBUTE_TCP_INFO *distributeTcpInfo;
PLAYBACK_TCP_INFO playbackTcpInfo;
pthread_mutex_t mute;

void server_start(threadpool_t *thp) {
    pid_t pid;
    pid = fork();
    if (pid == EXIT_FAIL_CODE) {
        LogWrite(ERROR, "%d %s :%s", __LINE__,
                 "dup-distribute fork error", strerror(errno));
        close(distributeTcpInfo[0].acceptfd);
    }
    if (pid == 0) {
        // 作为转发功能（作为server和client的双重角色转发）
        distribute_server(thp);
    } else {
        //监听
        //处理回放数据的功能（作为server接收态势的信息）
        playback_server(thp);
    }
}

void playback_server(threadpool_t *thp) {
    int playback_socketfd;
    int res;
    playback_socketfd = socket(AF_INET, SOCK_STREAM, (int)0);
    if (EXIT_FAIL_CODE == playback_socketfd) {
        LogWrite(ERROR, "%d %s :%s", __LINE__,
                 "playback socket failed", strerror(errno));
        _exit(EXIT_FAIL_CODE);
    }

    /* set no TIME_WAIT*/
    int playback_reuse = 1;
    res = setsockopt(playback_socketfd, SOL_SOCKET, SO_REUSEADDR, (void *)&playback_reuse, sizeof(int));
    if (res == EXIT_FAIL_CODE) {
        LogWrite(ERROR, "%d %s :%s", __LINE__,
                 "playback setsockopt failed", strerror(errno));
        close(playback_socketfd);
        _exit(EXIT_FAIL_CODE);
    }

    LogWrite(DEBUG, "%d %s", __LINE__, "playback socket created");
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(playbackTcpInfo.port);
    addr.sin_addr.s_addr = inet_addr(playbackTcpInfo.address);

    res = bind(playback_socketfd, (struct sockaddr*)&addr, sizeof(addr));
    if (EXIT_FAIL_CODE == res) {
        LogWrite(ERROR, "%d %s :%s", __LINE__,
                 "playbook bind failed", strerror(errno));
        close(playback_socketfd);
        _exit(EXIT_FAIL_CODE);
    }
    LogWrite(DEBUG, "%d %s", __LINE__, "playbook socket bind");

    res = listen(playback_socketfd, MAX_QUEUED_REQUESTS);
    if (EXIT_FAIL_CODE == res) {
        LogWrite(ERROR, "%d %s :%s", __LINE__,
                 "playbook listen failed", strerror(errno));
        close(playback_socketfd);
        _exit(EXIT_FAIL_CODE);
    }
    LogWrite(DEBUG, "%d %s", __LINE__, "playbook socket listening");

    int playbook_acceptfd = -1;
    while(1) {
        struct sockaddr_in caddr = {0};
        int csize = sizeof(caddr);
        playbook_acceptfd = accept(playback_socketfd, (struct sockaddr*)&caddr, (socklen_t *)&csize);
        if (EXIT_FAIL_CODE == playbook_acceptfd) {
            LogWrite(ERROR, "%d %s :%s", __LINE__, "playbook accept failed", strerror(errno));
            _exit(EXIT_FAIL_CODE);
        }

        LogWrite(INFO, "%d %s", __LINE__, "playbook connection ok");

        LogWrite(DEBUG, "%d %s: %d", __LINE__,
                 "distribute accept fd get", playbackTcpInfo.acceptfd);

        LogWrite(INFO, "%d %s%d %s%s", __LINE__,
                 "playbook cport=", ntohs(caddr.sin_port), "playbook caddr=", inet_ntoa(caddr.sin_addr));
        // use process
        pid_t pid;
        pid = fork();
        if (pid == EXIT_FAIL_CODE) {
            LogWrite(ERROR, "%d %s :%s", __LINE__,
                     "playbook fork error", strerror(errno));
            close(playbackTcpInfo.acceptfd);
            break;
        }
        if (pid == 0) {
            char sendbuf[MAX_BUFFER_SIZE] = {0};
            char recbuf[MAX_BUFFER_SIZE] = {0};
            static ReplayProtocol replayProtocol;
            while (1) {
                // child process
                bzero(&recbuf, sizeof(recv));
                res = recv(playbackTcpInfo.acceptfd, &recbuf, sizeof(recbuf), 0);
                if (EXIT_FAIL_CODE == res) {
                    LogWrite(ERROR, "%d %s :%s", __LINE__,
                             "playbook server receive error", strerror(errno));
                }
                // 从接收的消息数列中提取一个数据头信息出来
                memcpy(&replayProtocol, recbuf, sizeof(ReplayProtocol));
                if (replayProtocol.PacketHead == 0x88888888) {
                    printf("0000000000starttime: %d", replayProtocol.StartTime);
                }
                bzero(&sendbuf, sizeof(sendbuf));
                res = send(playbackTcpInfo.acceptfd, &sendbuf, sizeof(sendbuf), 0);
                if (EXIT_FAIL_CODE == res) {
                    printf("%s\n", "xxxxxxxxxxxxxxxxsend done");
                }
            }
        }
    }
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

	LogWrite(DEBUG, "%d %s", __LINE__, "master-client socket creating");
	for (int i = 1; i <= distributeTcpInfo[0].clientNum; i++) {
		int res = distribute_client_socket(i);
		if (res == EXIT_FAIL_CODE) {
			LogWrite(ERROR, "%d %s :%s:%d", __LINE__,
                     "master-client socket failed",
                     distributeTcpInfo[i].address, distributeTcpInfo[i].port);
            distributeTcpInfo[i].acceptfd = -1;
		} else {
			LogWrite(DEBUG, "%d %s :%s:%d %d", __LINE__,
                     "master-client socket created",
                     distributeTcpInfo[i].address, distributeTcpInfo[i].port, distributeTcpInfo[i].acceptfd);
		}
	}

	int distribute_acceptfd = -1;
	while(1) {
		struct sockaddr_in caddr = {0};
		int csize = sizeof(caddr);
        distribute_acceptfd = accept(distribute_socketfd, (struct sockaddr*)&caddr, (socklen_t *)&csize);
		if (EXIT_FAIL_CODE == distribute_acceptfd) {
			LogWrite(ERROR, "%d %s :%s", __LINE__, "distribute accept failed", strerror(errno));
			_exit(EXIT_FAIL_CODE);
		}
		if (distributeTcpInfo[0].acceptfd) {
			LogWrite(ERROR, "%d %s :%d", __LINE__,
                     "distribute socket fd already occupied by", distributeTcpInfo[0].acceptfd);
			LogWrite(ERROR, "%d %s", __LINE__,
                     "distribute only support one sender to server, close connection");
			close(distribute_acceptfd);
			continue;
		} else {
			LogWrite(INFO, "%d %s", __LINE__, "distribute connection ok");
		}

        distributeTcpInfo[0].acceptfd = distribute_acceptfd;
		LogWrite(DEBUG, "%d %s: %d", __LINE__,
                 "distribute accept fd get", distributeTcpInfo[0].acceptfd);

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
                     "master-client receive process created");
			distribute_receive(thp);
		}
        distributeTcpInfo[0].acceptfd = 0;
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
                 "master-client setsockopt failed", strerror(errno));
		close(socketfd);
		return EXIT_FAIL_CODE;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(distributeTcpInfo[index].port);
	addr.sin_addr.s_addr = inet_addr(distributeTcpInfo[index].address);

	res = connect(socketfd,(struct sockaddr*)&addr, sizeof(addr));
	if(EXIT_FAIL_CODE == res) {
		LogWrite(ERROR, "%d %s :%s", __LINE__, "master-client connect failed", distributeTcpInfo[index].address);
		return EXIT_FAIL_CODE;
	}
    distributeTcpInfo[index].acceptfd = socketfd;
	return EXIT_SUCCESS_CODE;
}

void distribute_receive(threadpool_t *thp) {
	int distribute_acceptfd;
    unsigned int uuid;
	int client_number = distributeTcpInfo[0].clientNum;
	pthread_t tids[client_number];
	unsigned char buf[MAX_BUFFER_SIZE] = {0};
    distribute_acceptfd = distributeTcpInfo[0].acceptfd;

	THREAD_PARAM thread_param;

	while(1) {
		int res;
        FILE *file = {0x0};
		bzero(&buf, sizeof(buf));
		res = recv(distribute_acceptfd, &buf, sizeof(buf), (int)0);

		if (EXIT_FAIL_CODE == res) {
			LogWrite(ERROR, "%d %s :%s", __LINE__, "distribute receive failed", strerror(errno));
			break;
		}
        else if (res > 0) {
			/*直接读buf会碰到0结束的情况*/
			LogWrite(INFO, "%d %s", __LINE__, "distribute received message");

            /*
                发送方给的数据就是一个字节的16进制数（0x89这类型），一16进制是4bit，也就是半字节。所以定义接收
                16进制数，主要得知道接收的每个16进制数的大小。
                char就是一个字节，unsigned char可以将打印出的16进的fff解决（是因为char是有符号的，16进制转换2进制头是1的话就会有fff）
            */
            file = initDataRecord(file, &uuid);
            if (file == NULL) {
                LogWrite(ERROR, "%d %s:", __LINE__, "distribute received message get failed");
                return;
            }

            /*
                记录传输数据
            */
            // 写入到一个文件中
            fwrite(buf, sizeof(char), res, file);
			for (int i = 0; i < res; i++) {
                //fprintf(file,"%c", buf[i]);
                if (i != 0 && i % 16 == 0) {
                    printf("\n");
                }
                printf("%02X ", (unsigned char)(buf[i]));
			}
            fclose(file);

            static SeaCommunication seaCommunication;
            static BDCommunication bdCommunication;

            // 从接收的消息数列中提取一个数据头信息出来
            memcpy(&seaCommunication,buf,sizeof(seaCommunication));
            memcpy(&bdCommunication,buf,sizeof(bdCommunication));

            if (seaCommunication.PacketHead == 0x66666666) {
                printf("\nSeaCommunication:\n");
                printf("PacketHead: %d\n", seaCommunication.PacketHead);
                printf("PayloadSize: %d\n", seaCommunication.PayloadSize);
                printf("ChunkNum: %d\n", seaCommunication.ChunkNum);
                printf("ChunkID: %d\n", seaCommunication.ChunkID);
                printf("DataTime: %d\n", seaCommunication.DataTime);
                printf("Longitude: %f\n", seaCommunication.Longitude);
                printf("Latitude: %f\n", seaCommunication.Latitude);
                printf("SystemID: %d\n", seaCommunication.SystemID);
                printf("DataType: %d\n", seaCommunication.DataType);
            } else if (bdCommunication.PacketHead == 0xEB) {
                printf("\nBdCommunication:\n");
                printf("PacketHead: %d\n", bdCommunication.PacketHead);
                printf("SystemID: %d\n", bdCommunication.SystemID);
                printf("DataType: %d\n", bdCommunication.DataType);
            } else {
                printf("Not SeaCommunication data or BdCommunication data\n");
            }
		} else if (res == 0) {
			break;
		}

		// strncpy在拷贝的时候，即使长度还没到，但是遇到0也会自动截断
		//strncpy(thread_param.buf, buf, sizeof(buf));
		memcpy(thread_param.buf, buf, res);
		thread_param.bufSize = res;
		pthread_mutex_init(&mute, NULL);
		LogWrite(DEBUG, "%d %s", __LINE__, "thread mutex init");
		for (int i = 0; i < client_number; i++) {
			//创建线程锁
			thread_param.clientIndex = i + 1;
			int ret = pthread_create(&tids[i], NULL,distribute_client_send, (void *)&thread_param);
			if (EXIT_FAIL_CODE == ret) {
				LogWrite(ERROR, "%d %s :%s", __LINE__, "master-client thread create failed", strerror(errno));
				close(distributeTcpInfo[i+1].acceptfd);
                distributeTcpInfo[i+1].acceptfd = 0;
			}
			for (int i = 0; i < client_number; i++) {
				pthread_join(tids[i], NULL);
			}

		}
		//释放锁
		LogWrite(DEBUG, "%d %s", __LINE__, "thread mutex destroy");
		pthread_mutex_destroy(&mute);
		bzero(&thread_param.buf, sizeof(thread_param.buf));
	}
	LogWrite(INFO, "%d %s", __LINE__, "distribute receive done");
    distributeTcpInfo[0].acceptfd = 0;
	close(distribute_acceptfd);
}

void *distribute_client_send(void *pth_arg) {
	pthread_mutex_lock(&mute);
	THREAD_PARAM *thread_param = (THREAD_PARAM *)pth_arg;
	LogWrite(DEBUG, "%d %s:%d", __LINE__, "thread locked by client", thread_param->clientIndex);
	char *buf = thread_param->buf;
	int index = thread_param->clientIndex;
	int bufSize = thread_param->bufSize;

	LogWrite(DEBUG, "%d %s :%d", __LINE__, "master-client thread created and acceptfd", distributeTcpInfo[index].acceptfd);

	int res = send(distributeTcpInfo[index].acceptfd, buf, bufSize, 0);
	if (EXIT_FAIL_CODE == res) {
		LogWrite(ERROR, "%d %s %s :%s:%d", __LINE__, "send [FAIL] to", distributeTcpInfo[index].address, strerror(errno), distributeTcpInfo[index].port);
	} else if(res > 0) {
		LogWrite(DEBUG, "%d %s %s:%d", __LINE__, "send [SUCCESS] to", distributeTcpInfo[index].address, distributeTcpInfo[index].port);
	}
	LogWrite(DEBUG, "%d %s:%d", __LINE__, "thread unlocked by client", thread_param->clientIndex);
	pthread_mutex_unlock(&mute);

	return pth_arg;
}

void parseFile_playbackTcpInfo(PLAYBACK_TCP_INFO *playbackTcpInfo) {
    char path[512] = {0x0};
    char **fileData = NULL;
    int lines = 0;

    getcwd(path, sizeof(path));
    strcat(path, CONFIGFILE);

    struct ConfigInfo *info = NULL;
    //加载配置文件
    loadFile_ConfigFile(path, &fileData, &lines);
    //解析配置文件
    parseFile_ConfigFile(fileData, lines, &info);
    memset(playbackTcpInfo, 0, sizeof(PLAYBACK_TCP_INFO));
    strcpy(playbackTcpInfo->address, getInfo_ConfigFile("playback-address", info, lines));
    playbackTcpInfo->port = atoi(getInfo_ConfigFile("playback-port", info, lines));

    LogWrite(DEBUG, "%d %s :%s", __LINE__, "playbook address is", playbackTcpInfo->address);
    LogWrite(DEBUG, "%d %s :%d", __LINE__, "playbook port is", playbackTcpInfo->port);
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