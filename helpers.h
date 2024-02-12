#ifndef HELPERS_H
#define HELPERS_H
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
// 256KB
#define SOCKET_READ_SIZE 256 * 1024
// 2MB
#define RESPONSE_SIZE 2 * 1024 * 1024
// 1MB
#define RESPONSE_BODY_SIZE 1024 * 1024

typedef struct sockaddr SA;
typedef struct sockaddr_in SA_IN;

#define SOCKET_ERROR -1

#define LOGGING 1

#define TRANSACTION_ERROR -1
#define TRANSACTION_SUCCESS 0

// Response templates
const char* successResponseJsonTemplate = "HTTP/1.1 200 OK\nContent-Type: application/json\n\n%s";
#define OK(clientSocket, response) send(clientSocket, response, strlen(response), 0);

// static responses
#define STATIC_RESPONSE(clientSocket, response) send(clientSocket, response, sizeof(response), 0);

const char badRequestResponse[] = "HTTP/1.1 400 Bad Request\nContent-Type: application/json\n\n{\"message\": \"Bad Request\"}";
#define BAD_REQUEST(clientSocket) STATIC_RESPONSE(clientSocket, badRequestResponse)

const char methodNotAllowedResponse[] = "HTTP/1.1 405 Method Not Allowed\nContent-Type: application/json\n\n{\"message\": \"Method not allowed\"}";
#define METHOD_NOT_ALLOWED(clientSocket) STATIC_RESPONSE(clientSocket, methodNotAllowedResponse)

const char notFoundResponse[] = "HTTP/1.1 404 Not Found\nContent-Type: application/json\n\n{\"message\": \"User Not Found\"}";
#define NOT_FOUND(clientSocket) STATIC_RESPONSE(clientSocket, notFoundResponse)

const char internalServerErrorResponse[] = "HTTP/1.1 500 Internal Server Error\nContent-Type: application/json\n\n{\"message\": \"Internal Server Error\"}";
#define INTERNAL_SERVER_ERROR(clientSocket) STATIC_RESPONSE(clientSocket, internalServerErrorResponse)

const char unprocessableEntityResponse[] = "HTTP/1.1 422 Unprocessable Entity\nContent-Type: application/json\n\n{\"message\": \"Unprocessable Entity\"}";
#define UNPROCESSABLE_ENTITY(clientSocket) STATIC_RESPONSE(clientSocket, unprocessableEntityResponse)

const char emptyOkResponse[] = "HTTP/1.1 200 OK\n";
#define EMPTY_OK(clientSocket) STATIC_RESPONSE(clientSocket, emptyOkResponse)

// HTTP methods
const char GET_METHOD[] = "GET";
const int GET_METHOD_LENGTH = sizeof(GET_METHOD) - 1;
const char POST_METHOD[] = "POST";
const int POST_METHOD_LENGTH = sizeof(POST_METHOD) - 1;

int check(int expression, const char* message);
int setupServer(short port, int backlog);
int partialCompare(const char* str1, const char* str2, int maxLength);

#define errIfNull(pointer) \
    if (pointer == NULL) { \
        return -1;         \
    }

// Check socket errors
int check(int expression, const char* message) {
    if (expression == SOCKET_ERROR) {
        perror(message);
        exit(EXIT_FAILURE);
    }
    return expression;
}

// Startup server socket on the given port, with the max number of connections waiting to be accepted set to backlog
int setupServer(short port, int backlog) {
    int serverSocket, clientSocket, addressSize;
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

// Compare two strings up to maxLength
int partialEqual(const char* str1, const char* str2, int maxLength) {
    for (int i = 0; i < maxLength; i++) {
        if (str1[i] == '\0' || str2[i] == '\0') {
            return false;
        }
        if (str1[i] != str2[i]) {
            return false;
        }
    }
    return true;
}

// GET /clientes/1/... 1 on the 14th position
int getIdFromGETRequest(const char* request, int requestLength) {
    if (requestLength < 15) {
        return -1;
    }
    if (request[13] != '/' || request[15] != '/') {
        return -1;
    }
    if (request[14] < '0' || request[14] > '9') {
        return -1;
    }
    return request[14] - '0';
}

// POST /clientes/1/... 1 on the 15th position
int getIdFromPOSTRequest(const char* request, int requestLength) {
    if (requestLength < 16) {
        return -1;
    }
    if (request[14] != '/' || request[16] != '/') {
        return -1;
    }
    if (request[15] < '0' || request[15] > '9') {
        return -1;
    }
    return request[15] - '0';
}

int findStartOfJson(const char* request, int requestLength) {
    for (int i = 0; i < requestLength; i++) {
        if (request[i] == '{') {
            return i;
        }
    }
    return -1;
}

#endif