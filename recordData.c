#include "recordData.h"

pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER; /*初始化互斥锁*/

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
    unsigned int timestamp = time(&t);

    strcat(path, "/");
    sprintf(charValue, "%d", timestamp);
    strcat(path, charValue);
    strcat(path, ".data");
}

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

    //创建文件
    CreateDataFile(path);
    temp = fopen(path, "w+");
    if (temp == NULL) {
        LogWrite(ERROR, "%d %s :%s", __LINE__, "fopen failed", strerror(errno));
        return EXIT_FAIL_CODE;
    }

    *file = temp;
    return EXIT_SUCCESS_CODE;
}