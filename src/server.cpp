#include "stdio.h"
#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <winsock2.h>
#pragma warning(disable: 4996)
using namespace std;

int main(int argc, char* argv[]) {
    int CountClient = 0;
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

    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
    bind(sListen, (SOCKADDR) &addr, sizeof(addr));
    listen(sListen, SOMAXCONN);

    SOCKET newConnection;
    newConnection=accept(sListen, (SOCKADDR*)&addr, &sizeof(addr)); // если ошибка, то int sizeofaddr = sizeof(addr)

    if(newConnection == 0) {
        cout << "Error connection player"
    } else {
        cout << "Client "<< CountClient++ + 1 <<" Connected!\n";
        char msg[256] = "Welcome to the TicTacToe server version 1.0.0!\n"+
        "You are the " + CountClient == 1 ? "first" : "second" + " player!\n";
        if(CountClient == 1) { // доделать!
            msg+= "Please wait for the other player to connect...\n"
        }
        send(newConnection, msg, sizeof(msg), NULL);

    }
    return 0;
}