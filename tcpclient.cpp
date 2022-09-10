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

bool check_h(char h){
	return (h >=0 && h < 24);
}

bool check_m_s(char m_s){
	return (m_s >= 0 && m_s < 60);
}

char *itoa(unsigned int number, int base) {
	int count = 0;
	char* destination = new char [10];
	do {
		int digit = number % base;
		destination[count++] = (digit > 9) ? digit - 10 +'A' : digit + '0';
	} while ((number /= base) != 0);
	destination[count] = '\0';
	int i;
	for (i = 0; i < count / 2; ++i) {
		char symbol = destination[i];
		destination[i] = destination[count - i - 1];
		destination[count - i - 1] = symbol;
	}
	return destination;
}

bool check_B(unsigned int B, char* B_str){
	char* b = new char [10];
	strcpy(b, itoa(B, 10));
	return (B_str[0] != '-' && !strcmp(b, B_str));
}

int send_file(FILE* file, int s){	
	char* line;
	unsigned int m_num = 0, rcvline = 0;
	while (1){
		unsigned long long mes_leng = 0;
		char c;
		while(1) {
			c = fgetc(file);
			if (c != '\n' && c != EOF) mes_leng++;
			else break;
		}
		if (mes_leng > 0){
			fseek(file, -mes_leng - 1, SEEK_CUR);
			line = (char*)calloc(mes_leng + 1, sizeof(char));
			fgets(line, mes_leng + 1, file);
        
			char* time = strtok(line, " ");
			char* time2 = strtok(NULL, " ");
			char* BBB_str = strtok(NULL, " ");
			unsigned int BBB = atoll(BBB_str);
			char* message = strtok(NULL, " ");
			*(message + strlen(message)) = '\n';
			
			char hh = atoi(strtok(time, ":"));
			char mm = atoi(strtok(NULL, ":"));
			char ss = atoi(strtok(NULL, ":"));
			char hh2 = atoi(strtok(time2, ":"));
			char mm2 = atoi(strtok(NULL, ":"));
			char ss2 = atoi(strtok(NULL, ":"));
			unsigned int m_num_h = htonl(m_num);
			
			if (!(check_h(hh) && check_m_s(mm) && check_m_s(ss) && check_h(hh2) && check_m_s(mm2) && check_m_s(ss2)&& check_B(BBB, BBB_str))) continue;
			
			BBB = htonl(BBB);
			send(s, &m_num_h, 4, MSG_NOSIGNAL);
			send(s, &hh, 1, MSG_NOSIGNAL);
			send(s, &mm, 1, MSG_NOSIGNAL);
			send(s, &ss, 1, MSG_NOSIGNAL);
			send(s, &hh2, 1, MSG_NOSIGNAL);
			send(s, &mm2, 1, MSG_NOSIGNAL);
			send(s, &ss2, 1, MSG_NOSIGNAL);
			send(s, &BBB, 4, MSG_NOSIGNAL);
			send(s, message, strlen(message), MSG_NOSIGNAL);
			
			m_num++;
			
			char* buffer;
			int res = recv(s, buffer, 2, 0);
			if(res > 0 && buffer[0] == 'o' && buffer[1] == 'k') rcvline++;
		}
        
        if (c == EOF) break;
	}
	return (m_num == rcvline);
}

int main(int argc, char* argv[])
{
	if (argc != 3) {
		cout << "Wrong number of arguments!" << endl;
		return 0;
	}

	int s;
	struct sockaddr_in addr;

	init();

	char* ip = (char*)calloc(16,sizeof(char));
	char* port = (char*)calloc(16,sizeof(char));
	char* tmp = (char*)calloc(32,sizeof(char));
	char* filename = (char*)calloc(64, sizeof(char));

	for (int i = strlen((char*)argv[1]); i>=0; --i)
		tmp[strlen((char*)argv[1])-i-1] = argv[1][i];

	ip = strtok((char*)argv[1], ":");
	strtok(tmp,":");
 
	for (int i = strlen(tmp); i>=0; --i)
		port[strlen(tmp)-i-1] = tmp[i];
	strcpy(filename,argv[2]);

	FILE* file = fopen(filename, "r");
	if(!file)
		cout << "File " << '\'' << filename << '\'' << " has not been opened" << endl; 

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return sock_err("socket", s);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(port));
	addr.sin_addr.s_addr = inet_addr(ip);

	int try_c = 0;
	cout<< "Connecting to " << ip << ':' << port << endl;
	while (connect(s, (struct sockaddr*) & addr, sizeof(addr)) != 0){
		if(try_c == 0)
			cout << "Connection error.\nRetrying..." << endl;

		try_c++;
		cout << "Try: " << try_c << endl;

		if (try_c > 9){
			s_close(s);
			deinit();
			return sock_err("connect", s);
		}

		sleep(0.1);
	}
	cout << "Connected." << endl;

	send(s, "put", 3, 0);

	if (send_file(file, s)) cout << "Data has been sent correctly." << endl;
	else cout << "Data has not been sent. Something went wrong." << endl;
    
	fclose(file);
	s_close(s);
	deinit();
	return 0;
}
