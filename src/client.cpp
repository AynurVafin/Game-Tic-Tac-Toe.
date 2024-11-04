#include "stdio.h"
#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <winsock2.h>

#pragma warning(disable: 4996)
using namespace std;

int main(int argc, char* argv[]) {
    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2,1);
    if(WSAStartup(DLLVersion,&wsaData) != 0) {
        cout << "ERROR" << endl;
        exit(1);
    }
    SOCKADDR_IN addr;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(argv[argc - 1]); // переделать !
    addr.sin_family = AF_INET;

    SOCKET Connection = socket(AF_INET, SOCK_STREAM, NULL);
    if(connect(Connection,(SOCKADDR*)&addr, sizeof(addr))!=0){
        cout << "Error: failed connect to server\n";
        return 1;
    }
    cout << "Connected\n";
    char msg[256];
    recv(Connection, msg, sizeof(msg), NULL);
    cout << msg << endl;

    return 0;
}