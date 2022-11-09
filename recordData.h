#include "log.h"
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <fcntl.h>

#define MODE_DIR (S_IRWXU | S_IRWXG | S_IRWXO)


FILE* initDataRecord(FILE * file);
void GetDirName(char *date);
