#include "log.h"
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <fcntl.h>

#define MODE_DIR (S_IRWXU | S_IRWXG | S_IRWXO)


int initDataRecord(FILE ** file);
/*static void CreateDataFile(char *path);
static unsigned int CreateDataDir(char *path);
void GetDirName(char *date);*/
/*typedef struct dataseting{
	char            filepath[MAXFILEPATH];
    char            datatime[MAXLOGTIME];
    FILE            *datafile;
}DATAGSET;*/

/*int dataWrite(char *format, ...);
static void PrintfData(char *format, va_list args);
static int InitData();
static void SetDataTime();
static DATAGSET* GetDataSet();
static unsigned char ReadDataConfig(char *path);*/
