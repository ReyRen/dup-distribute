#include "recordData.h"

pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER; /*初始化互斥锁*/

//DATAGSET datasetting;
/*
 * 存data的目录名是精确到日的时间：e.g.20221105
 * */
void GetDirName(char *date) {
    time_t timer = time(NULL);
    strftime(date, 11, "%Y%m%d", localtime(&timer));
}

static unsigned int CreateDataDir(char *path) {
    char value[512]={0x0};
    char data[50]={0x0};

    char **fileData = NULL;
    int lines = 0;
    struct ConfigInfo *info = NULL;
    //加载配置文件
    loadFile_ConfigFile(path, &fileData, &lines);
    //解析配置文件
    parseFile_ConfigFile(fileData, lines, &info);
    strcpy(value, getInfo_ConfigFile("data-path", info, lines));

    GetDirName(data); // 20221106
    strcat(value, "/");
    strcat(value, data); //./data/20221106

    if (opendir(value) == NULL) {
        // 没有该目录
        int ret = mkdir(value, MODE_DIR);//创建目录
        if (ret != 0) {
            LogWrite(ERROR, "%d %s :%s", __LINE__, "mkdir failed", strerror(errno));
            return EXIT_FAIL_CODE;
        }

    }
    memset(path, 0, 512);
    memcpy(path, value, strlen(value));

    return EXIT_SUCCESS_CODE;
}

static void CreateDataFile(char *path) {
    char charValue[512] = {0x0};

    //生成UTC时间戳文件
    time_t t;
    t = time(NULL);
    int timestamp = time(&t);

    strcat(path, "/");
    sprintf(charValue, "%d", timestamp);
    strcat(path, charValue);
    strcat(path, ".data");
}

/*static unsigned char ReadDataConfig(char *path){
    char value[512]={0x0};
    char data[50]={0x0};


    char **fileData = NULL;
    int lines = 0;
	struct ConfigInfo *info = NULL;
	//加载配置文件
	loadFile_ConfigFile(path, &fileData, &lines);
	//解析配置文件
	parseFile_ConfigFile(fileData, lines, &info);

    strcpy(value, getInfo_ConfigFile("data-path", info, lines));
    GetDirName(data);
    strcat(data, ".data");
    strcat(value, "/");
    strcat(value, data); // value: ./data/2022-09-15.log

    if (strcmp(value, datasetting.filepath) != 0) {
        memcpy(datasetting.filepath, value, strlen(value));
    }

    destroInfo_ConfigFile(info);

    return EXIT_SUCCESS_CODE;
}

static DATAGSET* GetDataSet() {
    char path[512] = {0x0};
    getcwd(path, sizeof(path));
    strcat(path, CONFIGFILE);

    if (access(path, F_OK) == 0) {
        ReadDataConfig(path);
    } else {
        memcpy(datasetting.filepath, "./data/unknown.data", 20);
    }

    return &datasetting;
}

static void SetDataTime() {
    time_t timer = time(NULL);
    strftime(datasetting.datatime, 20, "%Y-%m-%d %H:%M:%S", localtime(&timer));
}

static int InitData() {
    char strdate[30]={0x0};

    DATAGSET* datasetting;

    // 获取数据配置信息
    if ((datasetting = GetDataSet()) == NULL) {
		perror("Get data Set Fail!");
		return EXIT_FAIL_CODE;
	}

    SetDataTime();
    if (strlen(datasetting->filepath) == 0) {
        char *path = getenv("HOME");
        memcpy(datasetting->filepath, path, strlen(path));
        getdate(strdate);
        strcat(strdate, ".data");
        strcat(datasetting->filepath, "/");
        strcat(datasetting->filepath, strdate);
    }

    if(datasetting->datafile == NULL) {
        datasetting->datafile = fopen(datasetting->filepath, "a+"); // a+ 追加/创建 
    }
	if(datasetting->datafile == NULL){
		perror("Open data File Fail!");
		return EXIT_FAIL_CODE;
	}

    fprintf(datasetting->datafile, "[%s] ", datasetting->datatime);
    return EXIT_SUCCESS_CODE;
}

static void PrintfData(char *format, va_list args) {
    int d;
	char c, x, *s;

    while (*format)
    {
        switch (*format)
        {
        case 's':
            s = va_arg(args, char *);
            fprintf(datasetting.datafile, "%s", s);
            break;
        case 'd': {
            d = va_arg(args, int);
			fprintf(datasetting.datafile, "%d", d);
			break;
        }
        case 'c': {
            c = va_arg(args, int);
			fprintf(datasetting.datafile, "%c", c);
			break;
        }
        case 'x': {
            x = va_arg(args, int);
            fprintf(datasetting.datafile, "%#2X", x);
			break;
        }
        default:
            if (*format != '%' && *format != '\n') {
                fprintf(datasetting.datafile, "%c", *format); //如果是空格就也输入空格
            }
            break;
        }
        format++;
    }
}

int dataWrite(char *format, ...) {
    int rtv;
    va_list args;

    pthread_mutex_lock(&data_mutex);
    do {
        if (Initdata() != 0) {
            rtv = EXIT_FAIL_CODE;
            break;
        }

        va_start(args, format);
        PrintfData(format, args);
        va_end(args);

        fflush(datasetting.datafile);

        if (datasetting.datafile != NULL) {
            fclose(datasetting.datafile);
        }
        datasetting.datafile = NULL; // 防止野指针
        rtv = EXIT_SUCCESS_CODE;
    }while (0);
    
    pthread_mutex_unlock(&data_mutex);

   return rtv; 
}

void GetSequenceDate(char *date) {
    time_t timer = time(NULL);
    strftime(date, 11, "%Y-%m-%d", localtime(&timer));
}*/

int initDataRecord(FILE **file) {
    //创建目录
    FILE *temp;
    char path[512] = {0x0};
    getcwd(path, sizeof(path));
    strcat(path, CONFIGFILE);


    if (access(path, F_OK) == 0) {
        if (CreateDataDir(path) == EXIT_FAIL_CODE) {
            LogWrite(ERROR, "%d %s", __LINE__, "CreateDataDir failed");
            return EXIT_FAIL_CODE;
        }
    } else {
        LogWrite(ERROR, "%d %s :%s", __LINE__, "data directory create failed", strerror(errno));
        return EXIT_FAIL_CODE;
    }

    CreateDataFile(path);
    temp = fopen(path, "rwa+");
    if (temp == NULL) {
        LogWrite(ERROR, "%d %s :%s", __LINE__, "fopen failed", strerror(errno));
        return EXIT_FAIL_CODE;
    }

    *file = temp;
    return EXIT_SUCCESS_CODE;
}