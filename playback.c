#include "playback.h"
#include "util.h"

//
// Created by Yuan Ren on 2022/11/8.
//

void playback_run(unsigned char *receive_buf, int receive_size) {
    ReplayProtocol replayProtocol;
    bzero(&replayProtocol, sizeof(ReplayProtocol));

    memcpy(&replayProtocol, receive_buf, sizeof(ReplayProtocol));
/*
    unsigned int starttime = replayProtocol.StartTime;
    unsigned int endtime = replayProtocol.EndTime;
    unsigned int commandtype = replayProtocol.CommandType;
    unsigned int speed = replayProtocol.Speed;
*/
   // int speed = 5; // default
    printf("qqqqqqqqqqqqq: %s\n", receive_buf);
}