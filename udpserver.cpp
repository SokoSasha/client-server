#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib") 
#else// LINUX
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#endif

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cstdio>
#include <ctime>

using namespace std;

#define MAX_MES 20

int *ips;
int *ports;

typedef struct {
	int msgs;
	int msm[MAX_MES];
	int port;
	int ip;
	int sokk;
	time_t time;
} client;

int set_non_block_mode(int s){
#ifdef _WIN32
	unsigned long mode = 1;
	return ioctlsocket(s, FIONBIO, &mode);
#else
	int fl= fcntl(s, F_GETFL, 0);
	return fcntl(s, F_SETFL, fl | O_NONBLOCK);
#endif
}

int init()
{
    #ifdef _WIN32
        WSADATA wsa_data;
        return (0 == WSAStartup(MAKEWORD(2, 2), &wsa_data));
    #else
        return 1;
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

bool isin_ip(int ip, int n){
	for (int i = 0; i < n; i++) {
		if (ip == ips[i]) return 0;
		if (ips[i] == 0) {
			ips[i] = ip;
			return 1;
		}
	}
	return 1;
}

bool isin_port(int ip, int n){
	for (int i = 0; i < n; i++) {
		if (ip == ports[i]) return 0;
		if (ports[i] == 0) {
			ports[i] = ip;
			return 1;
		}
	}
	return 1;
}

bool isin_m(int num, int m_n[], int n){
	for (int i = 0; i < n; i++) if (m_n[i] == num) return 0;
	return 1;
}

int main(int argc, char* argv[])
{	
	if (argc != 3)
	{
		cout << "Wrong arguments!" << endl;
		return 0;
	}
	
	
	struct sockaddr_in addr;
	int port1 = atoi(argv[1]);
	int port2 = atoi(argv[2]);
	int num_socks = port2 - port1 + 1;
	ips = (int*)calloc(num_socks, sizeof(int));
	ports = (int*)calloc(num_socks, sizeof(int));
	client clnt[num_socks];
		
	
	printf("Listening to UDP ports %d-%d\n", port1, port2);
	
#ifdef _WIN32
	int flags = 0;
#else
	int flags = MSG_NOSIGNAL;
#endif

	// Инициалиазация сетевой библиотеки
	init();
	
	// Создание UDP-сокета
	for (int i = 0; i < num_socks; i++){
		clnt[i].sokk = socket(AF_INET, SOCK_DGRAM, 0);
		if (clnt[i].sokk < 0)
			return sock_err("socket", clnt[i].sokk);
		
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port1 + i); 
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		
		if (bind(clnt[i].sokk, (struct sockaddr*) & addr, sizeof(addr)) < 0)
			return sock_err("bind", clnt[i].sokk);
		
		set_non_block_mode(clnt[i].sokk);
		
		clnt[i].port = port1 + i;
		clnt[i].msgs = 0;
		clnt[i].time = 0;
		memset(&clnt[i].msm, -1, MAX_MES);
		
	}
	
	fd_set rfd;
	fd_set wfd;
	int nfds = clnt[0].sokk;
	struct timeval tv = { 1, 0 };
	bool flag = 1;
	while(flag) {
		FD_ZERO(&rfd);
		FD_ZERO(&wfd);
		for (int i = 0; i < num_socks; i++) {
			FD_SET(clnt[i].sokk, &rfd);
			FD_SET(clnt[i].sokk, &wfd);
			if (nfds < clnt[i].sokk)
				nfds = clnt[i].sokk;
		}
		if(select(nfds + 1, &rfd, &wfd, 0, &tv) > 0){
			for (int i = 0; i < num_socks; i++){
				if (FD_ISSET(clnt[i].sokk, &rfd) || FD_ISSET(clnt[i].sokk, &wfd)){
					char buffer[1050] = "";
					unsigned int addrlen = sizeof(addr);
					if (recvfrom(clnt[i].sokk, buffer, sizeof(buffer), flags, (struct sockaddr*)&addr, &addrlen) > 0){
						clnt[i].time = time(NULL);
						
						clnt[i].ip = ntohl(addr.sin_addr.s_addr);
						if (isin_ip(clnt[i].ip,  num_socks) && isin_port(clnt[i].port, num_socks))
							printf("Client %d.%d.%d.%d:%d detected\n", 
							(clnt[i].ip >> 24) & 0xFF, (clnt[i].ip >> 16) & 0xFF, (clnt[i].ip >> 8) & 0xFF, (clnt[i].ip) & 0xFF, clnt[i].port);
						
						int num_i = 0, hh1 = 0, mm1 = 0, ss1 = 0, hh2 = 0, mm2 = 0, ss2 = 0, num = 0;
						char message[1024] = "";
						memcpy(&num_i, buffer, 4);
						int num_i_h = ntohl(num_i);
						memcpy(&hh1, buffer + 4, 1);
						memcpy(&mm1, buffer + 5, 1);
						memcpy(&ss1, buffer + 6, 1);
						memcpy(&hh2, buffer + 7, 1);
						memcpy(&mm2, buffer + 8, 1);
						memcpy(&ss2, buffer + 9, 1);
						memcpy(&num, buffer + 10, 4);
						num = ntohl(num);
						strcat(message, buffer + 14);
						
						flag = strcmp(message, "stop");
						
						int msgs = clnt[i].msgs;
						
						
						if (isin_m(num_i, clnt[i].msm, msgs)){
							sendto(clnt[i].sokk, (const char*)&num_i, 4, flags, (struct sockaddr*)&addr, addrlen);
							clnt[i].msm[msgs] = num_i;
							clnt[i].msgs++;
							msgs++;
						
							FILE* file = fopen("msg.txt", "a");
							if (!file) {
								cout << "	File error!" << endl;
								return 0;
							}
							
							fprintf(file, "%u.%u.%u.%u:%d %02d:%02d:%02d %02d:%02d:%02d %u %s\n", 
								(clnt[i].ip >> 24) & 0xFF, (clnt[i].ip >> 16) & 0xFF, (clnt[i].ip >> 8) & 0xFF, (clnt[i].ip) & 0xFF, 
								clnt[i].port, hh1, mm1, ss1, hh2, mm2, ss2, num, message);
							
							fclose(file);
						}
						
						if (!flag) {
							cout << "Stop message was recieved. Stopping the server." << endl;
							for (int j = 0; j < num_socks; j++)
								sendto(clnt[j].sokk, "stop", 4, flags, (struct sockaddr*)&addr, addrlen);
							break;
						}
						
						if (clnt[i].msgs == MAX_MES){
							printf("%d messages recieved from client %d.%d.%d.%d:%d.\n", MAX_MES, 
								(clnt[i].ip >> 24) & 0xFF, (clnt[i].ip >> 16) & 0xFF, (clnt[i].ip >> 8) & 0xFF, 
								(clnt[i].ip) & 0xFF, clnt[i].port);
							
							break;
						}
					}
					else {
						time_t diff = time(NULL);
						if (clnt[i].time){
							diff = diff - clnt[i].time;
							if (diff > 30){
								printf("No data recieved from client %d.%d.%d.%d:%d for 30 seconds. Deleting client's info\n", 
									(clnt[i].ip >> 24) & 0xFF, (clnt[i].ip >> 16) & 0xFF, (clnt[i].ip >> 8) & 0xFF, (clnt[i].ip) & 0xFF, clnt[i].port);
								
								clnt[i].msgs = 0;
								clnt[i].time = 0;
								memset(&clnt[i].msm, -1, MAX_MES);
								clnt[i].time = 0;
								break;
							}
						}
						
					}
				}
			}
		}
		else {
			cout << "Error!" << endl;
		}
	}
	
	// Закрытие сокета
	for (int i = 0; i < num_socks; i++)
		s_close(clnt[i].sokk);
	deinit();
	return 0;
}
