#define _CRT_SECURE_NO_WARNINGS
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>

	#pragma comment(lib, "ws2_32.lib")

#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <string.h>
	#include <stdio.h>
	#include <errno.h>
	#include <unistd.h>
	#include <netdb.h>
#endif


#include <iostream>
#include <stdio.h>
#include <string.h>
using namespace std;

unsigned int ip;
unsigned port;

int init()
{
#ifdef _WIN32

	WSADATA wsa_data;
	return (0 == WSAStartup(MAKEWORD(2, 2), &wsa_data));
#else
	return 1;
#endif
}

int set_non_block_mode(int s)
{
#ifdef _WIN32
	unsigned long mode = 1;
	return ioctlsocket(s, FIONBIO, &mode);
#else
	int fl = fcntl(s, F_GETFL, 0);
	return fcntl(s, F_SETFL, fl | O_NONBLOCK);
#endif
}


void deinit()
{
#ifdef _WIN32

	WSACleanup();
#else

#endif
}


int sock_err(const char* function, int s)
{
	int err;
#ifdef _WIN32
	err = WSAGetLastError();
#else
	err = errno;
#endif

	fprintf(stderr, "%s: socket error: %d\n", function, err);
	return -1;
}


void s_close(int s)
{
#ifdef _WIN32
	closesocket(s);
#else
	close(s);
#endif
}




char* recv_fixed_line(int cs)
{
	char* buffer = (char*)calloc(5, sizeof(char));
	char* num1 = (char*)calloc(13, sizeof(char));
	char* num2 = (char*)calloc(13, sizeof(char));
	char* time_rcv = (char*)calloc(4, sizeof(char));
	char* times = (char*)calloc(9, sizeof(char));
	char* out = (char*)calloc(34, sizeof(char));

	recv(cs, buffer, 4, 0);
	buffer[4] = '\0';
	recv(cs, num1, 12, 0);
	num1[12] = '\0';
	recv(cs, num2, 12, 0);
	num2[12] = '\0';
	recv(cs, time_rcv, 3, 0);
	time_rcv[3] = '\0';
	times[0] = (int)time_rcv[0] / 10 + '0';
	times[1] = (int)time_rcv[0] % 10 + '0';
	times[2] = ':';
	times[3] = (int)time_rcv[1] / 10 + '0';
	times[4] = (int)time_rcv[1] % 10 + '0';
	times[5] = ':';
	times[6] = (int)time_rcv[2] / 10 + '0';
	times[7] = (int)time_rcv[2] % 10 + '0';
	times[8] = '\0';

	strcat(out, num1);
	out[12] = ' ';
	strcat(out, num2);
	out[25] = ' ';
	strcat(out, times);

	if (strcmp(out, times) == 0)
	{
		return NULL;
	}
	else
	{
		return out;
	}
}

short recv_string(int cs)
{
	FILE* file = fopen("msg.txt", "a");
	if (!file) {
		cout << "Could't open file!" << endl;
		exit(1);
	}
	short flag = 1;
	char put[4] = "\0";
	int m = 0;
	char nm = recv(cs, put, 3, 0);
	if (nm >= 0) do
		{
			unsigned int m_num;
			char hh;
			char mm;
			char ss;
			char hh2;
			char mm2;
			char ss2;
			unsigned int BBB = 0;
			char message[500000] = { '\0' };

			m = recv(cs, (char*)&m_num, 4, 0);
			m_num = ntohl(m_num);
			m += recv(cs, &hh, 1, 0);
			m += recv(cs, &mm, 1, 0);
			m += recv(cs, &ss, 1, 0);
			m += recv(cs, &hh2, 1, 0);
			m += recv(cs, &mm2, 1, 0);
			m += recv(cs, &ss2, 1, 0);
			m += recv(cs, (char*)&BBB, 4, 0);
			BBB = ntohl(BBB);

			int rcv = recv(cs, message, 1, 0);
			while (message[rcv - 1] != '\0' && message[rcv - 1] != '\n' && rcv < 500000)
			{
				rcv += recv(cs, message + rcv, 1, 0);
				if (rcv <= 0) {
					fclose(file);
					return flag;
				}
			}
			if (message[rcv - 1] != '\n') message[rcv - 1] = '\n';
			message[rcv] = '\0';

			flag = strcmp(message, "stop\n") && strcmp(message, "stop\0") && strcmp(message, "stop");

			fprintf(file, "%d.%d.%d.%d:%d %d:%d:%d %d:%d:%d %u %s", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF, port, hh, mm, ss, hh2, mm2, ss2, BBB, message);

			send(cs, "ok", 2, 0);

		} while (m >= 0);

	fclose(file);
	return flag;
}

int main(int argc, char* argv[])
{
	if (argc != 2){
		cout << "Wrong arguments!";
		return 0;
	}

	port = atoi(argv[1]);
	cout << "Listening TCP port: " << port << "..." << endl;

	int s;
	struct sockaddr_in addr;

	init();

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) return sock_err("socket", s);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) return sock_err("bind", s);

	if (listen(s, 1) < 0) return sock_err("listen", s);


	do {
		int addrlen = sizeof(addr);
		int cs = accept(s, (struct sockaddr*)&addr, &addrlen);
		int len;
		if (cs < 0){
			sock_err("accept", s);
			cout << "Error" << endl;
			break;
		}

		ip = ntohl(addr.sin_addr.s_addr);
		printf("Client connected: %d.%d.%d.%d\n", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF);

		short flag = recv_string(cs);

		printf("Client disconnected: %d.%d.%d.%d\n\n", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF);
		s_close(cs);
		
		if (!flag) {
			cout << "Message \"stop\" has been received. Stopping the server." << endl;
			return 0;
		}
	} while (1);

	return 0;
}