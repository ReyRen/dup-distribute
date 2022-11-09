#include "playback.h"
#include "util.h"

DISTRIBUTE_TCP_INFO *distributeTcpInfo;
//
// Created by Yuan Ren on 2022/11/8.
//

void playback_run(unsigned char *receive_buf, int receive_size) {
    ReplayProtocol replayProtocol;
    bzero(&replayProtocol, sizeof(ReplayProtocol));

    memcpy(&replayProtocol, receive_buf, sizeof(ReplayProtocol));

    unsigned int starttime = replayProtocol.StartTime;
    unsigned int endtime = replayProtocol.EndTime;
    unsigned int commandtype = replayProtocol.CommandType;
    unsigned int speed = replayProtocol.Speed;

    if (commandtype == PLAYBACK_START) {
        printf("\n");
        printf("\n");
        printf("\n");
        printf("IP: %s\n", distributeTcpInfo[0].address);
    } else if (commandtype == PLAYBACK_END) {
        printf("\n");
        printf("\n");
        printf("\n");
        printf("IP: %s\n", distributeTcpInfo[0].address);
    } else {
        LogWrite(ERROR, "%d %s", __LINE__,
                 "Wrong playback signal get");
        return;
    }
}