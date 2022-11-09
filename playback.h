//
// Created by Yuan Ren on 2022/11/8.
//
#include "dataHeader.h"
#include "log.h"

#ifndef DUP_DISTRIBUTE_PLAYBACK_H
#define DUP_DISTRIBUTE_PLAYBACK_H

void playback_run(unsigned char *receive_buf, int receive_size, int distribute_acceptfd);

#endif //DUP_DISTRIBUTE_PLAYBACK_H
