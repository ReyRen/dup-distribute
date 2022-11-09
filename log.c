#include "log.h"
#define MAXLEVELNUM (3)

LOGSET logsetting;
LOG loging;

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER; /*初始化互斥锁*/

const static char LogLevelText[4][10]={"INFO","DEBUG","WARN","ERROR"};

void GetDate(char *date) {
    time_t timer = time(NULL);
    sprintf(date, "%d-%d-%d",
            localtime(&timer)->tm_year,
            localtime(&timer)->tm_mon,
            localtime(&timer)->tm_mday);
}

unsigned char GetCode(char *path) {
    unsigned char code = 255;
    if (strcmp("INFO", path) == 0) {
        code = 1;
    }
    else if (strcmp("WARN", path) == 0)
    {
        code = 3;
    }
    else if (strcmp("ERROR", path) == 0)
    {
        code = 4;
    }
    else if (strcmp("NONE", path) == 0)
    {
        code = 0;
    }
    else if (strcmp("DEBUG", path) == 0)
    {
        code = 2;
    }

    return code;
}

static unsigned char ReadLogConfig(char *path){
    char value[512]={0x0};
    char data[50]={0x0};


    char **fileData = NULL;
    int lines = 0;
	struct ConfigInfo *info = NULL;
	//加载配置文件
	loadFile_ConfigFile(path, &fileData, &lines);
	//解析配置文件
	parseFile_ConfigFile(fileData, lines, &info);

    strcpy(value, getInfo_ConfigFile("log-path", info, lines));
    GetDate(data);
    strcat(data, ".log");
    strcat(value, "/");
    strcat(value, data); // value: ./temp/2022-09-15.log

    if (strcmp(value, logsetting.filepath) != 0) {
        memcpy(logsetting.filepath, value, strlen(value));
    }

    memset(value, 0, sizeof(value));

    strcpy(value, getInfo_ConfigFile("log-level", info, lines));
    logsetting.loglevel = GetCode(value);

    destroInfo_ConfigFile(info);

    return EXIT_SUCCESS_CODE;
}

/*
 *日志设置信息
 * */
static LOGSET* GetLogSet() {
    char path[512] = {0x0};
    getcwd(path, sizeof(path));
    strcat(path, CONFIGFILE);

    if (access(path, F_OK) == 0) {
        if (ReadLogConfig(path) != 0) {
            logsetting.loglevel = INFO;
            logsetting.maxfilelen = 4096;
        }
    } else {
        logsetting.loglevel = INFO;
        logsetting.maxfilelen = 4096;
    }

    return &logsetting;
}

static void SetTime() {
    time_t timer = time(NULL);
    sprintf(loging.logtime, "%d-%d-%d %d:%d:%d",
            localtime(&timer)->tm_year,
            localtime(&timer)->tm_mon,
            localtime(&timer)->tm_mday,
            localtime(&timer)->tm_hour,
            localtime(&timer)->tm_min,
            localtime(&timer)->tm_sec);
}

/*
 * 不定参打印
 * */
static void PrintfLog(char *format, va_list args) {
    int d;
	char c, *s;

    //int no_enter;

    while (*format)
    {
        switch (*format)
        {
        case 's':
            s = va_arg(args, char *);
            fprintf(loging.logfile, "%s", s);
            break;
        case 'd': {
            d = va_arg(args, int);
			fprintf(loging.logfile, "%d", d);
			break;
        }
        case 'c': {
            c = va_arg(args, int);
			fprintf(loging.logfile, "%c", c);
			break;
        }
        /*
        case 'x': {
            x = va_arg(args, int);
            fprintf(loging.logfile, "%#2X", x);
            no_enter = 1;
			break;
        }
        */
        default:
            if (*format != '%' && *format != '\n') {
                fprintf(loging.logfile, "%c", *format); //如果是空格就也输入空格
            }
            break;
        }
        format++;
    }
    fprintf(loging.logfile, "%s", "\n");
    /*
    if (!no_enter) {
        fprintf(loging.logfile, "%s", "\n");
    }
    */
}

static int InitLog(unsigned char loglevel){
    char strdate[30]={0x0};

    LOGSET* logsetting; // refer to the global logsetting

    // 获取日志配置信息
    if ((logsetting = GetLogSet()) == NULL) {
		perror("Get Log Set Fail!");
		return EXIT_FAIL_CODE;
	}

    if ((loglevel & (logsetting->loglevel)) != loglevel) { // 表示内存的loglevel不是log.conf中的loglevel
        return EXIT_FAIL_CODE;
    }

    memset(&loging, 0, sizeof(LOG));
    SetTime();
    if (strlen(logsetting->filepath) == 0) {
        char *path = getenv("HOME");
        memcpy(logsetting->filepath, path, strlen(path));
        GetDate(strdate);
        strcat(strdate, ".log");
        strcat(logsetting->filepath, "/");
        strcat(logsetting->filepath, strdate);
    }
    memcpy(loging.filepath, logsetting->filepath, MAXFILEPATH);

    if(loging.logfile == NULL) {
        loging.logfile = fopen(loging.filepath, "a+"); // a+ 追加/创建 
    }
	if(loging.logfile == NULL){
		perror("Open Log File Fail!");
		return EXIT_FAIL_CODE;
	}

    fprintf(loging.logfile, "[%s] [%s] ", LogLevelText[loglevel-1], loging.logtime);
    return EXIT_SUCCESS_CODE;
}

int LogWrite(unsigned char loglevel, char *format, ...) {
    int rtv;
    va_list args;

    pthread_mutex_lock(&log_mutex);
    do {
        if (InitLog(loglevel) != 0) {
            rtv = EXIT_FAIL_CODE;
            break;
        }

        va_start(args, format);
        PrintfLog(format, args);
        va_end(args);

        fflush(loging.logfile);

        if (loging.logfile != NULL) {
            fclose(loging.logfile);
        }
        loging.logfile = NULL; // 防止野指针
        rtv = EXIT_SUCCESS_CODE;
    }while (0);
    
    pthread_mutex_unlock(&log_mutex);

   return rtv; 
}