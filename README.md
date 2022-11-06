# dup-distribute
以一收多发作为核心基础功能.

### glibc替换为指定的（重写了scandir函数）
```asm
 mkdir glibc-2.27-build
./glibc-2.27/configure CFLAGS="-fno-builtin-strlen -ggdb -O2" FEATURES="preserve-libs nostrip splitdebug" --prefix=./glibc-2.27-build
make -j16
make install
```