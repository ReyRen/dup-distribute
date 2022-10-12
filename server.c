#define _CRT_SECURE_NO_WARNINGS
#include "util.h"
#include "ConfigFile.h"

//TCP_INFO *tcp_info;

int main(int argc, char ** argv)
{
	LogWrite(INFO, "%d %s", __LINE__, "*********Start Server*********");

	LogWrite(INFO, "%d %s", __LINE__, "parse config file to tcp_info");
	
	extern TCP_INFO *tcp_info;
	parseFile_tcpInfo(&tcp_info);

	LogWrite(INFO, "%d %s", __LINE__, "create tcp connection as server role");
	//tcp_server(tcp_info);
	tcp_server();

	return EXIT_SUCCESS_CODE;
}