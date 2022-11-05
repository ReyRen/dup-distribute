#include "log.h"
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <fcntl.h>

#define MODE_DIR (S_IRWXU | S_IRWXG | S_IRWXO)


int initDataRecord(FILE ** file);
