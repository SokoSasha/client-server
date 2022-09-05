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

int send_file(FILE* file, int s)
{	
	char* line;
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
			unsigned long long BBB = atoi(strtok(NULL, " "));
			char* message = strtok(NULL, " ");
			
			char hh = atoi(strtok(time, ":"));
			char mm = atoi(strtok(NULL, ":"));
			char ss = atoi(strtok(NULL, ":"));
			
			send(s, hh, 1, 0);
			send(s, mm, 1, 0);
			send(s, ss, 1, 0);
			send(s, BBB, sizeof(BBB), 0);
			send(s, message, sizeof(message), 0);
			
			cout << hh << endl << mm << endl << ss << endl << BBB << endl << message << endl;
		}
        
        if (c == EOF) break;
	}
	return 1;

		/*if (line[0] != '\n')
		{
            numline++;

			tr.num = htonl(numline);    //number of message
			send(s, tr.data, 4, MSG_NOSIGNAL);
            num += 4;

            ltmp2 = strtok(ltmp1," "); // the first phone number
            send(s, ltmp2, 12, MSG_NOSIGNAL);
            num += 12;

            ltmp2 = strtok (NULL, " "); // the second phone number
            send(s, ltmp2, 12, MSG_NOSIGNAL);
            num += 12;

            ltmp2 = strtok (NULL, ":"); 
            hh[0] = (ltmp2[0] - '0')*10;
            hh[0] += (ltmp2[1] - '0');
            send(s, hh, 1, MSG_NOSIGNAL);
            num++;
            
            ltmp2 = strtok (NULL, ":"); 
            mm[0] = (ltmp2[0] - '0')*10;
            mm[0] += (ltmp2[1] - '0');
            send(s, mm, 1, MSG_NOSIGNAL);
            num++;

                
            ltmp2 = strtok (NULL, " "); 
            ss[0] = (ltmp2[0] - '0')*10;
            ss[0] += (ltmp2[1] - '0');
            send(s, ss, 1, MSG_NOSIGNAL);
            num++; 
            
            fgets(message, 8192, file);
            if(message[strlen(message)-1] == '\n')
                message[strlen(message)-1] = '\0';
            send(s, message, strlen(message), MSG_NOSIGNAL);
            num += strlen(message);

            while(strlen(message) == 8191)
            {
                fgets(message, 8192, file);
                if(message[strlen(message)-1] == '\n')
                    message[strlen(message)-1] = '\0';
                send(s, message, strlen(message), MSG_NOSIGNAL);
                num+= strlen(message);
            }
            
            send(s, endel, 1, MSG_NOSIGNAL);
            num++;
            sntline++;
            cout << "  " << num / 1024 << " Kb and " << num - (num/1024)*1024 << " bytes sent" << endl;

            cout << "Waiting for response..." << endl;
            res = recv(s, buffer, 2, 0);

            if(res > 0)
                {
                    cout << "  " << res << " bytes recieved" << endl << endl;
                    if(buffer[0] == 'o' && buffer[1] == 'k')
                    {
                        rcvline++;
                    }
                }
		}

        res = 0;
        buffer = (char*)calloc(2, sizeof(char));
        message = (char*)calloc(8192,sizeof(char));
        ltmp1 = (char*)calloc(512, sizeof(char));
        ltmp2 = (char*)calloc(512, sizeof(char));
        num = 0;
	}

    if(rcvline == sntline)
    {
        return 1;
    }
    else
    {
        return 0;
    }*/
}




int main(int argc, char* argv[])
{
	if (argc != 3){
		cout << "Check your command" << endl;
		return -1;
	}

	int s;
	struct sockaddr_in addr;
    
	init();
    
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return sock_err("socket", s);
		
	char* filename = (char*)calloc(64, sizeof(char));

    char* ip = strtok(argv[1], ":");
	int port = atoi(strtok(NULL, " "));
    strcpy(filename,argv[2]);
    
    memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);

    FILE* file = fopen(filename, "r");
    if(!file)
        cout << "Something is wrong with the file \"" << filename << "\"" << endl;

	unsigned char num_of_tries = 0;
	
    cout<< "Connecting to " << ip << ':' << port << endl;
    
	while (connect(s, (struct sockaddr*) & addr, sizeof(addr)) != 0){
        if(num_of_tries == 0)
            cout << "Waiting..." << endl;

		//num_of_tries++;
		if (num_of_tries++ > 9){
			s_close(s);
			deinit();
			return sock_err("connect", s);
		}

		sleep(0.1);
	}
    cout << "Connected!" << endl;
	char put[] = "put";
    send(s, put, 3, 0);

    if(send_file(file, s)) cout << "Send!" << endl;
    else cout << "Error. Can't send data :(" << endl;
	
    s_close(s);
    deinit();
    return 0;
}
