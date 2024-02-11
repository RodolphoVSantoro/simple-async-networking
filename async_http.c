#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT 3000
// max connections waiting to be accepted
#define SERVER_BACKLOG 10
#define SOCKET_READ_SIZE 256 * 1024
// #define SOCKET_READ_SIZE 32
typedef struct sockaddr SA;
typedef struct sockaddr_in SA_IN;
#define SOCKET_ERROR -1

int setupServer(short port, int backlog);
int check(int expression, const char* message);

int setupServer(short port, int backlog) {
    int serverSocket, clientSocket, addressSize;
    // check((serverSocket = socket(AF_INET, SOCK_STREAM, 0)), "Failed to create socket");
    check((serverSocket = socket(AF_INET, SOCK_STREAM, 0)), "Failed to create socket");

    SA_IN serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    //  To avoid "Address already in use" error when restarting the server because of the TIME_WAIT state
    // https://handsonnetworkprogramming.com/articles/bind-error-98-eaddrinuse-10048-wsaeaddrinuse-address-already-in-use/
    int yes = 1;
    check(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)), "Failed to set socket options");

    check(bind(serverSocket, (SA*)&serverAddress, sizeof(serverAddress)), "Failed to bind socket");
    check(listen(serverSocket, backlog), "Failed to listen on socket");

    return serverSocket;
}

int check(int expression, const char* message) {
    if (expression == SOCKET_ERROR) {
        perror(message);
        exit(EXIT_FAILURE);
    }
    return expression;
}

int main() {
    int serverSocket = setupServer(SERVER_PORT, SERVER_BACKLOG);
    printf("Server is running(%d)\n", serverSocket);
    printf("Listening on port %d\n", SERVER_PORT);
    printf("FD_SETSIZE: %d\n", FD_SETSIZE);

    // int clientFd = accept(serverSocket, NULL, NULL);

    // char buffer[SOCKET_READ_SIZE];
    // int bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
    // if (bytesRead < 1) {
    //     close(clientFd);
    //     return -1;
    // }
    // buffer[bytesRead] = '\0';
    // printf("Received %s\n(%d bytes)", buffer, bytesRead);
    // const char* response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
    // send(clientFd, response, strlen(response), 0);

    // close(clientFd);
    // close(serverSocket);

    fd_set currentSockets, readySockets;

    FD_ZERO(&currentSockets);
    FD_SET(serverSocket, &currentSockets);

    const char* testResponse = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
    const char* overFlowResponse = "HTTP/1.1 400 Bad Request\nContent-Type: text/plain\nContent-Length: 12\n\nBad request!";

    while (true) {
        readySockets = currentSockets;

        if (select(FD_SETSIZE, &readySockets, NULL, NULL, NULL) < 0) {
            perror("Select failed");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &readySockets)) {
                if (i == serverSocket) {
                    struct sockaddr_in clientAddress;
                    socklen_t clientAddressSize = sizeof(clientAddress);
                    int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
                    FD_SET(clientSocket, &currentSockets);
                } else {
                    char buffer[SOCKET_READ_SIZE];
                    int bytesRead = recv(i, buffer, sizeof(buffer), 0);

                    if (bytesRead >= SOCKET_READ_SIZE - 1) {
                        send(i, overFlowResponse, strlen(overFlowResponse), 0);
                    } else if (bytesRead >= 1) {
                        buffer[bytesRead] = '\0';
                        printf("Received:\n---------\n%s\n---------\n(%d bytes)\n", buffer, bytesRead);
                        send(i, testResponse, strlen(testResponse), 0);
                    }

                    close(i);
                    FD_CLR(i, &currentSockets);
                }
            }
        }
    }

    close(serverSocket);
    return 0;
}