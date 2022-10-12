#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/*
char *c1 = "ABCEEF12345678900C0B"; //待测试字符串
uint8_t s[7] = {0xAB, 0xCC, 100, 50, 10, 9, 8}; //待测试16进制数组
*/

char lowtocap(char c);
void AsciiToHex(char *src, uint8_t *dest, int len);
void HexToAscii(uint8_t *src, char *dest, int len);
void decimalToBinary(int num);