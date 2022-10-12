#include "jinzhiConvert.h"

/* lowercase letters transform to capital letter*/
char lowtocap(char c) {
    if ((c >= 'a') && (c <= 'z')) {
        c = c - 32;
    }
    return c;
}

/* ascii string transform to 16 hex*/
void AsciiToHex(char *src, uint8_t *dest, int len) {
    int dh, dl; // 16进制的高4位和低4位
    char ch, cl; // 字符串的高位和地位
    int i;
    if (src == NULL || dest == NULL){
        printf("src or dest is NULL\n");
        return;
    }

    if (len < 1) {
        printf("length is NULL\n");
        return;
    }

    for (i = 0; i < len; i++) {
        ch = src[2 * i];
        cl = src[2 * i + 1];
        dh = lowtocap(ch) - '0';
        if (dh > 9) {
            dh = lowtocap(ch) - 'A' + 10;
        }
        dl = lowtocap(cl) - '0';
        if (dl > 9) {
            dl = lowtocap(cl) - 'A' + 10;
        }
        dest[i] = dh * 16 + dl;
        if (len%2 > 0) { //字符串个数为奇数
            dest[len / 2] = src[len-1] - '0';
            if (dest[len / 2] > 9) {
                dest[len/2] = lowtocap(src[len-1]) - 'A' + 10;
            }
        }
    }
}

/*16 jinzhi Hex transform to ascii string*/
void HexToAscii(uint8_t *src, char *dest, int len) {
    char dh,dl;  //字符串的高位和低位
    int i;
    if(src == NULL || dest == NULL) {
        printf("src or dest is NULL\n");
        return;
    }

    if(len < 1) {
        printf("length is NULL\n");
        return;
    }

    for(i = 0; i < len; i++) {
        dh = '0' + src[i] / 16;
        dl = '0' + src[i] % 16;
        if(dh > '9') {
            dh = dh - '9' - 1 + 'A'; // 或者 dh= dh+ 7;
        }
        if(dl > '9') {
            dl = dl - '9' - 1 + 'A'; // 或者dl = dl + 7;
        }
        dest[2*i] = dh;
        dest[2*i+1] = dl;
    }
    dest[2*i] = '\0';
}

// this method will print all the 32 bits of a number
void decimalToBinary(int num) {
    // assuming 32 bit integer
    for (int i = 31; i >= 0; i--) {
        
        // calculate bitmask to check whether
        // ith bit of num is set or not
        int mask = (1 << i);
        
        // ith bit of num is set 
        if (num & mask)
           printf("1");
        // ith bit of num is not set   
        else 
           printf("0");
    }
}
/*
int main(void) {
    int i, len;
    uint8_t temp[30] = {0};
    char temp1[50] = {0};

    AsciiToHex(c1, temp, strlen(c1));
    if (strlen(c1)%2 > 0) {
        len = strlen(c1)/2 + 1;
    } else {
        len = strlen(c1)/2;
    }

    for( i = 0; i < len; i++) {
        printf("int[%d] is %X,%d\n", i+1,temp[i],temp[i]);
    }

    HexToAscii(s,temp1,sizeof(s));
    printf("temp1 is %s", temp1);
char buf[4096] = {0};
printf("\n");
    printf("%02X\n", "1111111111111111");
    return 0;
    
}
*/
