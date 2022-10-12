#define _CRT_SECURE_NO_WARNINGS
//防止头文件重复包含
#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define BUFFERSIZE 					(1024)
#define CONFIGFILE 					"/engine.conf"

// exit code
#define EXIT_FAIL_CODE              -1
#define EXIT_SUCCESS_CODE           0

struct ConfigInfo {
	char key[64];
	char val[128];
};

// 目的为了在C++中能够调用C写的函数
#ifdef __cplusplus
    extern "C"{
#endif
    //获得文件有效行数
	int getLines_ConfigFile(FILE *file);
	//加载配置文件
	void loadFile_ConfigFile(const char *filePath, char ***fileData, int *lines);
	//解析配置文件
	void parseFile_ConfigFile(char **fileData, int lines, struct ConfigInfo **info);
	//获得指定配置信息
	char* getInfo_ConfigFile(const char *key, struct ConfigInfo *info, int line);
	//释放配置文件信息
	void destroInfo_ConfigFile(struct ConfigInfo *info);
	//判断当前行是否有效
	int isValid_ConfigFile(const char *buf);
#ifdef __cplusplus
	}
#endif