#include "ConfigFile.h"

void print_err(char *str, int line, int err_no) {
	printf("%d, %s :%s\n", line, str, strerror(err_no));
	_exit(EXIT_FAIL_CODE);
}

//获得文件有效行数
int getLines_ConfigFile(FILE *file) {
    char buf[BUFFERSIZE] = { 0 };
    int lines = 0;
    while (fgets(buf, BUFFERSIZE, file) != NULL) {
        if (!isValid_ConfigFile(buf)) {
            continue;
        }
        memset(buf, 0, BUFFERSIZE);
        ++lines;
    }

    //把文件指针重置到文件的开头, 因为还在用同一个文件流
    fseek(file, 0, SEEK_SET);

    return lines;
}

//加载配置文件
void loadFile_ConfigFile(const char *filePath, char ***fileData, int *line) {
    FILE *file = fopen(filePath, "r");
    if (NULL == file) {
        print_err("config file fopen failed", __LINE__, errno);
	}

    int lines = getLines_ConfigFile(file);

    //给每行数据开辟内存, 有多少行，就malloc多少个指针
    char **temp = malloc(sizeof(char *) * lines);
    char buf[BUFFERSIZE] = { 0 };
    int index = 0;
    while (fgets(buf, BUFFERSIZE, file) != NULL) {
        //如果返回false
        if (!isValid_ConfigFile(buf))
		{
			continue;
		}
        // +1是为了将'\0'放在最后
        temp[index] = malloc(strlen(buf) + 1);
        strcpy(temp[index], buf);
        index++;
        //清空buf
        memset(buf, 0, BUFFERSIZE);
    }

    fclose(file);
    
    *fileData = temp;
    *line = lines;
}
//解析配置文件
void parseFile_ConfigFile(char **fileData, int lines, struct ConfigInfo **info) {
    struct ConfigInfo *myinfo = malloc(sizeof(struct ConfigInfo) * lines);
    memset(myinfo, 0, sizeof(struct ConfigInfo) *lines);

    for (int i = 0; i < lines; i++) {
        /*C 库函数 char *strchr(const char *str, int c) 在参数 str 所指向的字符串中搜索第一次出现字符 c（一个无符号字符）的位置*/
        char *pos = strchr(fileData[i], '=');
        strncpy(myinfo[i].key, fileData[i], pos - fileData[i]);

        int flag = 0;
        if (fileData[i][strlen(fileData[i]) - 1] == '\n') {
            flag = 1;
        }
        strncpy(myinfo[i].val, pos + 1, strlen(pos + 1) - flag);
    }

    //释放文件信息
    for (int i = 0; i < lines; i++) {
        if (fileData[i] != NULL) {
            free(fileData[i]);
            fileData[i] = NULL;
        }
    }
    *info = myinfo;
}

//获得指定配置信息
char* getInfo_ConfigFile(const char *key, struct ConfigInfo *info, int line) {
    for (int i = 0; i < line; i++) {
        if (strcmp(key, info[i].key) == 0) {
            return info[i].val;
        }
    }
    return NULL;
}

//释放配置文件信息
void destroInfo_ConfigFile(struct ConfigInfo *info) {

	if (NULL == info) {
		return;
	}
	free(info);
	info = NULL;

}

//判断当前行是否有效
int isValid_ConfigFile(const char *buf) {
    //注释和空行不算入有效行
    if (buf[0] == '#' || buf[0] == '\n' || strchr(buf,'=') == NULL) {
		return 0;
	}
    return 1;
}