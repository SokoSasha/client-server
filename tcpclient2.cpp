#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib") 
#else // LINUX
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
#include <cstdlib>
#include <stdio.h>
#include <string.h>

using namespace std;

int ip = 0, port = 0;

int init(){
	#ifdef _WIN32
	// Для Windows следует вызвать WSAStartup перед началом использования сокетов
	WSADATA wsa_data;
	return (0 == WSAStartup(MAKEWORD(2, 2), &wsa_data));
	#else
	return 1; // Для других ОС действий не требуется
	#endif
}

void deinit(){
	#ifdef _WIN32
	// Для Windows следует вызвать WSACleanup в конце работы 
	WSACleanup(); 
	#else
	// Для других ОС действий не требуется
	#endif
}

int sock_err(const char* function, int s){
	int err;
	#ifdef _WIN32
	err= WSAGetLastError();
	#else
	err = errno;
	#endif
	
	fprintf(stderr, "%s: socket error: %d\n", function, err);
	return -1;
}

void s_close(int s){
	#ifdef _WIN32
	closesocket(s);
	#else
	close(s);
	#endif
}

int recv_response(int s, FILE* file){
	int res = 0;
	while(1){
		res = 0;
		unsigned int num = 0;
		char hh = 0;
		char mm = 0;
		char ss = 0;
		char hh2 = 0;
		char mm2 = 0;
		char ss2 = 0;
		unsigned int BBB = 0;
		char message[350000] = "";
		
		res = recv(s, (char*)&num, 4, 0);
		if (res <= 0) break;
		
		num = ntohl(num);
		recv(s, &hh, 1, 0);
		recv(s, &mm, 1, 0);
		recv(s, &ss, 1, 0);
		recv(s, &hh2, 1, 0);
		recv(s, &mm2, 1, 0);
		recv(s, &ss2, 1, 0);
		recv(s, (char*)&BBB, 4, 0);
		BBB = ntohl(BBB);
		
		
		int rcv = recv(s, message, 1, 0);
		while (message[rcv - 1] != '\0' && message[rcv - 1] != '\n')
		{
			rcv += recv(s, message + rcv, 1, 0);
			if (rcv <= 0) {
				fclose(file);
				return 0;
			}
		}

		fprintf(file, "%u.%u.%u.%u:%d %02d:%02d:%02d %02d:%02d:%02d %u %s\n", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF, port, hh, mm, ss, hh2, mm2, ss2, BBB, message);
		
	}
	if (res < 0)
		return sock_err("recv", s);
	return 0;
}

int main(int argc, char* argv[])
{
	if (argc != 4) {
		cout << "Wrong number of arguments!" << endl;
		return 0;
	}
	
	char* ipc = strtok(argv[1], ":");
	int portc = atoi(strtok(NULL, " "));
	char filename[40] = "";
	strcpy(filename,argv[3]);
	
	int s;
	struct sockaddr_in addr;
	FILE* file = fopen(filename, "a");
	
	init();
	
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return sock_err("socket", s);
	
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(portc);
	addr.sin_addr.s_addr = inet_addr(ipc);
	
	cout<< "Connecting to " << ipc << ':' << portc << endl;
	
	int try_c = 0;
	while (connect(s, (struct sockaddr*) & addr, sizeof(addr)) != 0){
		if(try_c == 0)
			cout << "Connection error.\nTrying again..." << endl;

		try_c++;
		cout << "Try number " << try_c << endl;

		if (try_c > 9){
			s_close(s);
			deinit();
			return sock_err("connect", s);
		}

		sleep(0.1);
	}
	ip = ntohl(addr.sin_addr.s_addr);
	port = ntohs(addr.sin_port);
	
	cout << "Connected." << endl;
	
	send(s, "get", 3, 0);
	
	recv_response(s, file);
	
	cout << "File received." << endl;
	
	fclose(file);
	s_close(s);
	deinit();
	return 0;
}
