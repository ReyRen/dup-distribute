#include "playback.h"
#include "util.h"

DISTRIBUTE_TCP_INFO *distributeTcpInfo;
//
// Created by Yuan Ren on 2022/11/8.
//

void playback_run(unsigned char *receive_buf, int receive_size, int distribute_acceptfd) {
    ReplayProtocol replayProtocol;
    bzero(&replayProtocol, sizeof(ReplayProtocol));

    memcpy(&replayProtocol, receive_buf, sizeof(ReplayProtocol));

    //unsigned int starttime = replayProtocol.StartTime;
    //unsigned int endtime = replayProtocol.EndTime;
    unsigned int commandtype = replayProtocol.CommandType;
    //unsigned int speed = replayProtocol.Speed;

    if (commandtype == PLAYBACK_START) {

        //scandirAndSend(starttime, endtime)
    } else if (commandtype == PLAYBACK_END) {
        LogWrite(INFO, "%d %s", __LINE__,
                 "playback get STOP signal, now close connection");
        close(distributeTcpInfo[0].acceptfd);
    } else {
        LogWrite(ERROR, "%d %s", __LINE__,
                 "Wrong playback signal get");
    }
}

