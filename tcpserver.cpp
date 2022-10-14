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
#include <vector>
using namespace std;

#define MAX_CLI 100

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

struct client {
	int cs;
	int ip;
	int port;
	char putget;
};

int main(int argc, char* argv[])
{
	/*if (argc != 2){
		cout << "Wrong arguments!";
		return 0;
	}*/

	//unsigned port_s = atoi(argv[1]);
	unsigned int port_s = 9000;
	cout << "Listening TCP port: " << port_s << "..." << endl;

	int s;
	struct sockaddr_in addr;

	init();

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return sock_err("socket", s);

	set_non_block_mode(s);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_s);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);


	if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		return sock_err("bind", s);

	if (listen(s, 1) < 0)
		return sock_err("listen", s);

	struct pollfd pfd[MAX_CLI + 1];
	pfd[MAX_CLI].fd = s; //прослущивающий порт
	pfd[MAX_CLI].events = POLLIN;

	client clnt[MAX_CLI]; //"база данных" о клиентах и ее заполнение
	for (int i = 0; i < MAX_CLI; i++)
	{
		pfd[i].fd = clnt[i].cs;
		pfd[i].events = POLLIN | POLLOUT;
	}

	if (listen(s, 1) < 0) return sock_err("listen", s);

	int CLI = -1; //число активных клиентов-1 (по совместительству индекс)
	while (1) {

#ifdef _WIN32
		int ev_cnt = WSAPoll(pfd, sizeof(pfd) / sizeof(pfd[0]), MAX_CLI);
		Sleep(1);
		//cout << ev_cnt << endl;
#else
		int ev_cnt = poll(pfd, sizeof(pfd) / sizeof(pfd[0]), MAX_CLI)
#endif

			if (ev_cnt > 0) {
				if (CLI != -1) for (int i = 0; i <= CLI; i++) {
					if (pfd[i].revents & POLLHUP)
					{
						// Сокет cs[i] - клиент отключился. Можно закрывать сокет
						printf("Client disconnected: %d.%d.%d.%d:%d\n\n", (clnt[i].ip >> 24) & 0xFF, (clnt[i].ip >> 16) & 0xFF, (clnt[i].ip >> 8) & 0xFF, (clnt[i].ip) & 0xFF, clnt[i].port);
						s_close(clnt[i].cs); //Для закрытия сокета также необходимо чистить информацию о нем, чтобы программа не пыталась работать с "мертвыми" клиентами
						for (int j = i; j < CLI - 1; j++) {
							clnt[j] = clnt[j + 1];
							pfd[j] = pfd[j + 1];
						}
						clnt[CLI - 1] = { 0,0,0,0 };
						pfd[CLI - 1] = { 0,0,0 };
						CLI--;
					}
					if (pfd[i].revents & POLLERR)
					{
						// Сокет cs[i] - возникла ошибка. Можно закрывать сокет
						printf("Error! Client disconnected: %d.%d.%d.%d:%d\n\n", (clnt[i].ip >> 24) & 0xFF, (clnt[i].ip >> 16) & 0xFF, (clnt[i].ip >> 8) & 0xFF, (clnt[i].ip) & 0xFF, clnt[i].port);
						s_close(clnt[i].cs);
						for (int j = i; j < CLI - 1; j++) {
							clnt[j] = clnt[j + 1];
							pfd[j] = pfd[j + 1];
						}
						clnt[CLI - 1] = { 0,0,0,0 };
						pfd[CLI - 1] = { 0,0,0 };
						CLI--;
					}
					if ((pfd[i].revents & POLLIN) && clnt[i].putget == 1) //работа с клиентами, отправляющими сообщения
					{
						// Сокет cs[i] доступен на чтение, можно вызывать recv/recvfrom
						//Считываем отправленные клиентом сообщения. Считывание происходит по 1 сообщению за раз, чтобы другие клиенты тоже могли пользоваться сервером
						//Открытие и закрытие файлов происходит в рамках одного сообщения для того, чтобы не произошло ошибок в работе с ними
						FILE* file = fopen("msg.txt", "a"), * nums = fopen("nums.txt", "a");
						if (!file || !nums) {
							cout << "Could't open file!" << endl;
							return 0;
						}

						bool flag = 1;
						int m = 0;

						unsigned int m_num = 0;
						char hh;
						char mm;
						char ss;
						char hh2;
						char mm2;
						char ss2;
						unsigned int BBB = 0;
						char message[350000] = "";

						m = recv(clnt[i].cs, (char*)&m_num, 4, 0);
						m_num = ntohl(m_num);
						m += recv(clnt[i].cs, &hh, 1, 0);
						m += recv(clnt[i].cs, &mm, 1, 0);
						m += recv(clnt[i].cs, &ss, 1, 0);
						m += recv(clnt[i].cs, &hh2, 1, 0);
						m += recv(clnt[i].cs, &mm2, 1, 0);
						m += recv(clnt[i].cs, &ss2, 1, 0);
						m += recv(clnt[i].cs, (char*)&BBB, 4, 0);
						BBB = ntohl(BBB);

						int rcv = recv(clnt[i].cs, message, 1, 0);
						while (message[rcv - 1] != '\0' && message[rcv - 1] != '\n')
						{
							rcv += recv(clnt[i].cs, message + rcv, 1, 0);
							if (rcv <= 0) {
								s_close(clnt[i].cs);
								for (int j = i; j < CLI - 1; j++) {
									clnt[j] = clnt[j + 1];
									pfd[j] = pfd[j + 1];
								}
								clnt[CLI - 1] = { 0,0,0,0 };
								pfd[CLI - 1] = { 0,0,0 };
								CLI--;
								fclose(file);
								fclose(nums);
							}
						}

						flag = strcmp(message, "stop\n") && strcmp(message, "stop\0") && strcmp(message, "stop");

						fprintf(file, "%d.%d.%d.%d:%d %02d:%02d:%02d %02d:%02d:%02d %u %s\n", (clnt[i].ip >> 24) & 0xFF,
							(clnt[i].ip >> 16) & 0xFF, (clnt[i].ip >> 8) & 0xFF, (clnt[i].ip) & 0xFF, clnt[i].port, hh, mm, ss, hh2, mm2, ss2, BBB, message);
						fprintf(nums, "%u\n", m_num);

						send(clnt[i].cs, "ok", 2, 0);
						fclose(file);
						fclose(nums);

						if (!flag) {
							printf("'stop' message was reveived. Stopping the server...\n");
							for (int j = 0; j < CLI; j++)
								s_close(clnt[j].cs);
							return 0;
						}

					}
					if ((pfd[i].revents & POLLOUT) && clnt[i].putget == -1) //работа с клиентами, ждущими сообщения. На самом деле и те, и другие клиенты могут как принимать, так и отправлять сообщения,
						//поэтому деление на POLLIN и POLLOUT происходит лишь формально
					{
						// Сокет cs[i] доступен на запись, можно вызывать send/sendto
						//В отличие от PUT клиентов чтение из файла происходит сразу для всех сообщений в файле, чтобы передать их разом, а не бесконечно принимать и отправлять по одному сообщению от разных клиентов
						FILE* nums = fopen("nums.txt", "r"), * file = fopen("msg.txt", "r");
						if (!file || !nums) {
							cout << "Could't open file!" << endl;
							return 0;
						}
						do {
							char line[350000] = "";
							if (feof(file)) break;
							fgets(line, 350000, file);
							if (strlen(line) < 20) continue;

							char* address = strtok(line, " ");
							char* time = strtok(NULL, " ");
							char* time2 = strtok(NULL, " ");
							char* BBB_str = strtok(NULL, " ");
							unsigned int BBB = atoll(BBB_str);
							char* message = strtok(NULL, "\n");

							char hh = atoi(strtok(time, ":"));
							char mm = atoi(strtok(NULL, ":"));
							char ss = atoi(strtok(NULL, ":"));
							char hh2 = atoi(strtok(time2, ":"));
							char mm2 = atoi(strtok(NULL, ":"));
							char ss2 = atoi(strtok(NULL, ":"));
							char numm[11] = "";
							fgets(numm, 11, nums);
							int num = htonl(atoi(numm));

							BBB = htonl(BBB);
							send(clnt[i].cs, (char*)&num, 4, 0);
							send(clnt[i].cs, &hh, 1, 0);
							send(clnt[i].cs, &mm, 1, 0);
							send(clnt[i].cs, &ss, 1, 0);
							send(clnt[i].cs, &hh2, 1, 0);
							send(clnt[i].cs, &mm2, 1, 0);
							send(clnt[i].cs, &ss2, 1, 0);
							send(clnt[i].cs, (char*)&BBB, 4, 0);
							send(clnt[i].cs, message, strlen(message) + 1, 0);
						} while (!feof(file) && !feof(nums));

						clnt[i].putget = 0;
						printf("File sent\n");
						s_close(clnt[i].cs);
						for (int j = i; j < CLI - 1; j++) {
							clnt[j] = clnt[j + 1];
							pfd[j] = pfd[j + 1];
						}
						clnt[CLI - 1] = { 0,0,0,0 };
						pfd[CLI - 1] = { 0,0,0 };
						CLI--;

						fclose(file);
						fclose(nums);
					}
				}
			}
		if (pfd[MAX_CLI].revents & POLLIN)
		{
			// Сокет ls доступен на чтение - можно вызывать accept, принимать
			// новое подключение. Новый сокет следует добавить в cs и создать для
			// него структуру в pfd.

			//Добавляем новые сокеты в нашу "базу данных"
			int addrlen = sizeof(addr);
			int cs_n = accept(s, (struct sockaddr*)&addr, &addrlen);
			//set_non_block_mode(cs_n);
			unsigned int ip_n = ntohl(addr.sin_addr.s_addr);
			unsigned int port_n = ntohs(addr.sin_port);
			printf("Client connected: %d.%d.%d.%d:%d\n", (ip_n >> 24) & 0xFF, (ip_n >> 16) & 0xFF, (ip_n >> 8) & 0xFF, (ip_n) & 0xFF, port_n);

			char req[4] = "";
			char nm = recv(cs_n, req, 3, 0);
			bool flag = 1;
			if (!strcmp(req, "put") || !strcmp(req, "get")) {
				clnt[++CLI].cs = cs_n;
				pfd[CLI].fd = cs_n;
				clnt[CLI].ip = ip_n;
				clnt[CLI].port = port_n;
				if (!strcmp(req, "put")) clnt[CLI].putget = 1; //в "БД" создается пометка о том, какого рода это клиент
				if (!strcmp(req, "get")) clnt[CLI].putget = -1;
			}
		}
	}
}