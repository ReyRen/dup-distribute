#include "log.h"



typedef struct dataseting{
	char            filepath[MAXFILEPATH];
    char            datatime[MAXLOGTIME];
    FILE            *datafile;
}DATAGSET;

int dataWrite(char *format, ...);
static void PrintfData(char *format, va_list args);
static int InitData();
static void SetDataTime();
static DATAGSET* GetDataSet();
static unsigned char ReadDataConfig(char *path);
