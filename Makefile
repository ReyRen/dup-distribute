#
# by yuanren 2022/09/08
#

NY: clean all
CC=gcc
CFLAGS=-Werror -Wall -g -D_GNU_SOURCE -D__USE_XOPEN -D_MIME_TYPE_
BIN_SERVER=dd-server
BIN_CLIENT=dd-client
BIN_SENDER=dd-sender
all:$(BIN_SERVER) $(BIN_CLIENT) $(BIN_SENDER)
%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@
dd-server:server.o util.o log.o ConfigFile.o jinzhiConvert.o recordData.o playback.o
	$(CC) $(CFLAGS) $^ -pthread -o $@
dd-sender:sender.o util.o log.o ConfigFile.o jinzhiConvert.o recordData.o playback.o
	$(CC) $(CFLAGS) $^ -pthread -o $@
dd-client:client.o util.o log.o ConfigFile.o jinzhiConvert.o recordData.o playback.o
	$(CC) $(CFLAGS) $^ -pthread -o $@
clean:
	rm -f *.o $(BIN_SERVER) $(BIN_CLIENT) $(BIN_SENDER)