#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#endif

#include <stdio.h>
#include <string.h>
#include <iostream>

using namespace std;

#define MAX_MES 20

unsigned int mes_num[MAX_MES];
int n = 0;
int n_q = 0;

int init() {
#ifdef _WIN32
	WSADATA wsa_data;
	return (0 == WSAStartup(MAKEWORD(2, 2), &wsa_data));
#else
	return 1;
#endif
}


void deinit() {
#ifdef _WIN32
	WSACleanup();
#else

#endif
}

void s_close(int s) {
#ifdef _WIN32
	closesocket(s);
#else
	close(s);
#endif
}

int sock_err(const char* function, int s) {
	int err;
#ifdef _WIN32
	err = WSAGetLastError();
#else
	err = errno;
#endif
	fprintf(stderr, "%s: socket error: %d\n", function, err);
	return -1;
}

bool itsin(unsigned int i) {
	for (char j = 0; j < n; j++) if (mes_num[j] == i) return 0;
	return 1;
}

void recv_response(int s);

void send_file(int s, struct sockaddr_in* addr, char* filename)
{
	FILE* file = fopen(filename, "r");

	if (file) {
		unsigned i = -1;
		while (!feof(file) && mes_num[n - 1] == -1) {
			i++;

			char message[350000] = "", datagram[350000] = "";
			int hh1 = 0, mm1 = 0, ss1 = 0, hh2 = 0, mm2 = 0, ss2 = 0;
			unsigned int num = 0;

			fscanf(file, "%d:%d:%d %d:%d:%d %u %s", &hh1, &mm1, &ss1, &hh2, &mm2, &ss2, &num, message);

			int I = htonl(i);
			num = htonl(num);
			memcpy(datagram, (char*)&I, 4);
			datagram[4] = hh1;
			datagram[5] = mm1;
			datagram[6] = ss1;
			datagram[7] = hh2;
			datagram[8] = mm2;
			datagram[9] = ss2;
			memcpy(datagram + 10, (const char*)&num, 4);
			strcat(datagram + 14, message);

#ifdef _WIN32
			int flags = 0;
#else
			int flags = MSG_NOSIGNAL;
#endif

			recv_response(s);
			if (itsin(i) && mes_num[n - 1] == -1) {
				printf("%d message(s) in queue\n", n_q);
				int res = sendto(s, datagram, 16 + strlen(message), flags, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
				if (res <= 0)
					sock_err("sendto", s);
				//recv_response(s);
			}

		}
	}
	else cout << "File error!" << endl;
	fclose(file);
}

// Функция принимает дейтаграмму от удаленной стороны.
// Возвращает 0, если в течение 100 миллисекунд не было получено ни одной дейтаграммы
void recv_response(int s)
{
	char datagram[80] = "";
	struct timeval tv = { 0, 100 * 1000 }; // 100 msec
	int res;

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(s, &fds);

	res = select(s + 1, &fds, 0, 0, &tv);
	if (res > 0)
	{
		// Данные есть, считывание их
		struct sockaddr_in addr;
		int addrlen = sizeof(addr);
		int received = recvfrom(s, datagram, sizeof(datagram), 0, (struct sockaddr*)&addr, &addrlen);
		if (!strcmp(datagram, "stop")) {
			s_close(s);
			exit(0);
		}
		if (received <= 0)
		{
			// Ошибка считывания полученной дейтаграммы
			sock_err("recvfrom", s);
			return;
		}
		int num = datagram[0] * 1000 + datagram[1] * 100 + datagram[2] * 10 + datagram[3];
		printf("Message #%d was sent\n", num);
		//num = ntohl(num);
		//memcpy(&num, (int)datagram, 4);
		for (int i = 0; i < n; i++) {
			if (mes_num[i] == num) {
				n_q--;
				return;
			}
			if (mes_num[i] == -1) {
				mes_num[i] = num;
				n_q--;
				return;
			}
		}
	}
	else if (res == 0)
	{
		// Данных в сокете нет, возврат ошибки
		return;
	}
	else
	{
		sock_err("select", s);
		return;
	}
}

int main(int argc, char* argv[])
{
	if (argc != 3) {
		cout << "Wrong arguments!";
		return 0;
	}

	int s;
	struct sockaddr_in addr;

	char* ip = strtok(argv[1], ":");
	int port = atoi(strtok(NULL, " "));
	char* filename = argv[2];

	/*char ip[] = "192.168.5.128";
	int port = 9140;
	char filename[] = "cli1.txt";*/

	FILE* file = fopen(filename, "r");
	while (!feof(file)) {
		char buff[1024] = "";
		fscanf(file, "%s %s %s %s", buff, buff, buff, buff);
		n++;
	}
	fclose(file);
	//n--;
	if (n > MAX_MES) n = MAX_MES;
	n_q = n;

	for (int i = 0; i < n; i++) mes_num[i] = -1;

	init();
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		return sock_err("socket", s);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);

	while (mes_num[n - 1] == -1) {
		send_file(s, &addr, filename);
	}
	printf("%d messages have been sent\n", n);
	// Закрытие сокета
	s_close(s);
	deinit();
	return 0;
}