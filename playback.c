#include "playback.h"
#include "util.h"
#include "recordData.h"

DISTRIBUTE_TCP_INFO *distributeTcpInfo;
//
// Created by Yuan Ren on 2022/11/8.
//

int filter_fn(const struct dirent *ent, char *start, char *end) {
    if (ent->d_type == DT_DIR) {
        LogWrite(ERROR, "%d %s", __LINE__,
                 "scandir get dir file, skip");
        return 0;
    }
    char realTime[10] = {0x0};
    strncpy(realTime, (char *)ent->d_name, 10);

    if (atoi(start) <= atoi(realTime) && atoi(end) >= atoi(realTime)) {
        return 1;
    } else {
        return 0;
    }
}

int scanAndSend(char *path, char *starttime,
                    char *endtime,
                    int index,
                    unsigned int speed) {
    int n;
    struct dirent **namelist;
    char realPath[1024] = {0x0};

    n = scandir(path, &namelist, filter_fn, starttime, endtime, alphasort);
    if (n < 0) {
        LogWrite(ERROR, "%d %s", __LINE__,
                 "scandir get error");
    } else {
        time_t t = time(NULL);
        unsigned int playtime = time(&t);

        int elapse = 0;
        for (int i = 0; i < n; ++i) {
            strcpy(realPath, path);
            strcpy(realPath + strlen(path), "/");
            strcpy(realPath + strlen(path) + 1, namelist[i]->d_name);

            //free(namelist[i]);
            printf("realPath: %s\n", realPath);
            FILE * stream;
            stream = fopen(realPath, "rb");
            if (stream == NULL) {
                LogWrite(ERROR, "%d %s", __LINE__,
                         " scanAndSend open file get error");
            }

            // 将光标停在文件的末尾
            long lSize;
            char* buffer;
            fseek (stream , 0 , SEEK_END);
            lSize = ftell (stream);
            rewind (stream);
            buffer = malloc(lSize);
            fread (buffer,1,lSize, stream);
            fclose(stream);


            /*
             * 倍数逻辑
             * */
            time_t t_now = time(NULL);
            unsigned int now = time(&t_now);
            elapse = now - playtime;

            char fileTime[10] = {0x0};
            strncpy(fileTime, (char *)namelist[i]->d_name, 10);
            if ((atoi(fileTime) - atoi(starttime)) > speed * elapse) {
                int res = send(distributeTcpInfo[index].acceptfd, buffer, lSize, 0);
                if (EXIT_FAIL_CODE == res) {
                    if (errno == EINTR ||errno == EAGAIN ||errno == EWOULDBLOCK) {
                        LogWrite(ERROR, "%d %s %s :%d", __LINE__,
                                 "playback send [FAIL] to fd, but link is OK, continue",
                                 strerror(errno), distributeTcpInfo[index].acceptfd);
                    } else {
                        LogWrite(INFO, "%d %s %s :%d", __LINE__,
                                 "playback send [FAIL] to fd, link is bad, exit playback mode",
                                 strerror(errno), distributeTcpInfo[index].acceptfd);
                        free(buffer);
                        memset(realpath, 0, sizeof(realPath));
                        return EXIT_FAIL_CODE;
                    }
                } else if(res > 0) {
                    LogWrite(DEBUG, "%d %s :%s", __LINE__, "playback send [SUCCESS] ", realPath);
                }
            } else {
                printf("time overflow: %s\n", fileTime);

            }
            usleep(50);

            free(buffer);
            memset(realpath, 0, sizeof(realPath));
            sleep(1);
        }
    }

    return EXIT_SUCCESS_CODE;
}


int myscandirServe(unsigned int starttime,
                   unsigned int endtime,
                   int index,
                   unsigned int speed) {

    char start_time_buffer[50];
    char end_time_buffer[50];
    sprintf(start_time_buffer, "%d", starttime);
    sprintf(end_time_buffer, "%d", endtime);

    // 获取当前日期
    char path[512] = {0x0};
    getcwd(path, sizeof(path));
    strcat(path, CONFIGFILE);

    char value[512]={0x0};
    char data[50]={0x0};
    char **fileData = NULL;
    int lines = 0;
    struct ConfigInfo *info = NULL;
    //加载配置文件
    loadFile_ConfigFile(path, &fileData, &lines);
    //解析配置文件
    parseFile_ConfigFile(fileData, lines, &info);
    strcpy(value, getInfo_ConfigFile("data-path", info, lines));

    GetDirName(data); // 20221106
    strcat(value, "/");
    strcat(value, data); //./data/20221106

    if (access(value, F_OK) == 0) {
        return scanAndSend(value,
                           start_time_buffer,
                           end_time_buffer,
                           index,
                           speed);
    } else {
        LogWrite(INFO, "%d %s", __LINE__,
                 "selected playback day not the today");
        return EXIT_FAIL_CODE;
    }
}

void playback_run(unsigned char *receive_buf, int receive_size, int index) {
    char shmFile[128] = {0x0};
    strcat(shmFile, "./shm/");
    strcat(shmFile, distributeTcpInfo[index].address);

    ReplayProtocol replayProtocol;
    memset(&replayProtocol, 0, sizeof(ReplayProtocol));

    memcpy(&replayProtocol, receive_buf, receive_size);

    unsigned int starttime = replayProtocol.StartTime;
    unsigned int endtime = replayProtocol.EndTime;
    unsigned int commandtype = replayProtocol.CommandType;
    unsigned int speed = replayProtocol.Speed;

    printf("starttime: %d\n", starttime);
    printf("endtime: %d\n", endtime);
    printf("commandtype: %d\n", commandtype);
    printf("speed: %d\n", speed);

    if (commandtype == PLAYBACK_START) {
        // 创建共享文件
        if (creat(shmFile, 0755) < 0) {
            LogWrite(ERROR, "%d %s", __LINE__,
                     "playback shm file create failed");
            return;
        }

        LogWrite(INFO, "%d %s", __LINE__,
                 "playback get START signal");

        int res = myscandirServe(starttime, endtime, index, speed);
        if (res == EXIT_SUCCESS_CODE) {
            LogWrite(INFO, "%d %s", __LINE__,
                     "playback send success");
        } else {
            LogWrite(INFO, "%d %s", __LINE__,
                     "playback send failed");
            if (remove(shmFile)) {
                LogWrite(ERROR, "%d %s: %s", __LINE__,
                         "playback shm file remove failed", strerror(errno));
                return;
            } else {
                LogWrite(DEBUG, "%d %s", __LINE__,
                         "playback shm file remove success");
            }
        }
    } else if (commandtype == PLAYBACK_END) {
        LogWrite(INFO, "%d %s", __LINE__,
                 "playback get STOP signal, now close connection");
        if (remove(shmFile)) {
            LogWrite(ERROR, "%d %s", __LINE__,
                     "playback shm file remove failed");
            return;
        } else {
            LogWrite(DEBUG, "%d %s", __LINE__,
                     "playback shm file remove success");
        }
    } else {
        LogWrite(ERROR, "%d %s", __LINE__,
                 "Wrong playback signal get");
        if (remove(shmFile)) {
            LogWrite(ERROR, "%d %s", __LINE__,
                     "playback shm file remove failed");
            return;
        } else {
            LogWrite(DEBUG, "%d %s", __LINE__,
                     "playback shm file remove success");
        }
    }
}

