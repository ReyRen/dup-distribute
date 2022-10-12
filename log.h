#ifndef __LOG_H__
#define __LOG_H__
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <unistd.h>
#include "ConfigFile.h"

#define MAXLEN          (2048)
#define MAXFILEPATH     (512)
#define MAXFILENAME     (50)
#define MAXLOGTIME      (20)

//pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER; /*初始化互斥锁*/

typedef enum{
	NONE    =   0,
	INFO    =   1,
	DEBUG   =   2,
	WARN    =   3,
	ERROR   =   4,
	ALL     =   255
}LOGLEVEL;

typedef struct log{
	char            logtime[MAXLOGTIME];
	char            filepath[MAXFILEPATH];
	FILE            *logfile;
}LOG;

typedef struct logseting{
	char            filepath[MAXFILEPATH];
	unsigned int    maxfilelen;
	unsigned char   loglevel;
}LOGSET;


int LogWrite(unsigned char loglevel, char *format, ...);

#endif
