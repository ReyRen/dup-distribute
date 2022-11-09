#include "playback.h"
#include "util.h"
#include "recordData.h"

DISTRIBUTE_TCP_INFO *distributeTcpInfo;
//
// Created by Yuan Ren on 2022/11/8.
//

int filter_fn(const struct dirent *ent, char *start, char *end) {
    if (ent->d_type != DT_REG) {
        LogWrite(ERROR, "%d %s", __LINE__,
                 "scandir get not regular file");
        return EXIT_FAIL_CODE;
    }
    char delims[] = "_";
    char *realTime = NULL;
    realTime = strtok( (char *)ent->d_name, delims );

    if (atoi(start) <= atoi(realTime) && atoi(end) >= atoi(realTime)) {
        return EXIT_SUCCESS_CODE;
    } else {
        return EXIT_FAIL_CODE;
    }
}

int scanAndSend(char *path, char *starttime,
                    char *endtime,
                    int distribute_acceptfd,
                    unsigned int speed) {
    int n;
    struct dirent **namelist;

    n = scandir(path, &namelist, fliter_fn, starttime, endtime, alphasort);
    if (n < 0) {
        LogWrite(ERROR, "%d %s", __LINE__,
                 "scandir get error");
    } else {
        for (int i = 0; i < n; ++i) {
            strcat(path, "/");
            strcat(path, namelist[i]->d_name);
            FILE * stream;
            stream = fopen(path, "r");
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

            int res = send(distribute_acceptfd, buffer, lSize, 0);
            if (EXIT_FAIL_CODE == res) {
                LogWrite(ERROR, "%d %s %s :%d", __LINE__, "playback send [FAIL] to fd",
                         strerror(errno), distribute_acceptfd);
            } else if(res > 0) {
                LogWrite(DEBUG, "%d %s :%d", __LINE__, "playback send [SUCCESS] to fd", distribute_acceptfd);
            }

            free(buffer);
        }
        close(distribute_acceptfd);
    }

    return EXIT_SUCCESS_CODE;
}


int myscandirServe(unsigned int starttime,
                   unsigned int endtime,
                   int distribute_acceptfd,
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
        return scanAndSend(value, start_time_buffer,
                               end_time_buffer,
                               distribute_acceptfd,
                               speed);
    } else {
        LogWrite(INFO, "%d %s", __LINE__,
                 "selected playback day not the today");
        return EXIT_FAIL_CODE;
    }
}

void playback_run(unsigned char *receive_buf, int receive_size, int distribute_acceptfd) {
    ReplayProtocol replayProtocol;
    bzero(&replayProtocol, sizeof(ReplayProtocol));

    memcpy(&replayProtocol, receive_buf, sizeof(ReplayProtocol));

    unsigned int starttime = replayProtocol.StartTime;
    unsigned int endtime = replayProtocol.EndTime;
    unsigned int commandtype = replayProtocol.CommandType;
    unsigned int speed = replayProtocol.Speed;

    if (commandtype == PLAYBACK_START) {
        LogWrite(INFO, "%d %s", __LINE__,
                 "playback get START signal");
       // int res = myscandirServe(starttime, endtime, distribute_acceptfd, speed);
        int res = myscandirServe(1667991918, 1667992970, distribute_acceptfd, speed);
        if (res == EXIT_SUCCESS_CODE) {
            LogWrite(INFO, "%d %s", __LINE__,
                     "playback get send done");
        } else {
            LogWrite(INFO, "%d %s", __LINE__,
                     "playback send failed");
        }
    } else if (commandtype == PLAYBACK_END) {
        LogWrite(INFO, "%d %s", __LINE__,
                 "playback get STOP signal, now close connection");
        close(distribute_acceptfd);
    } else {
        LogWrite(ERROR, "%d %s", __LINE__,
                 "Wrong playback signal get");
    }
}

