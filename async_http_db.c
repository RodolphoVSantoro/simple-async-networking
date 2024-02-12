#include "dbFiles.h"
#include "helpers.h"

void setGetResponse(User* user, char* responseBuffer);
void setPostResponse(User* user, char* responseBuffer);
int handleGetRequest(int clientSocket, char* buffer, int bufferSize);
int handlePostRequest(int clientSocket, char* buffer, int bufferSize);
int handleRequest(char* buffer, int bytesRead, int i);
int getTransactionFromPOSTRequest(char* buffer, int bufferSize, Transaction* transaction);

// Get transaction from body
// returns -1 if the body it fails to parse the body
// returns 0 if it parses the body successfully
// Sets the transaction variable with the parsed values
int getTransactionFromPOSTRequest(char* buffer, int bufferSize, Transaction* transaction) {
    char* body = strstr(buffer, "{");
    errIfNull(body);
    body += 1;

    // Find valor key in body
    char* valor = strstr(body, "valor");
    errIfNull(valor);
    valor = strstr(valor, ":");
    errIfNull(valor);
    valor++;
    transaction->valor = atoi(valor);

    // Find tipo key in body
    char* tipo = strstr(body, "tipo");
    errIfNull(tipo);
    tipo = strstr(tipo, ":");
    errIfNull(tipo);
    tipo = strstr(tipo, "\"");
    errIfNull(tipo);
    transaction->tipo = tipo[1];

    // Find descricao key in body
    char* descricaoStart = strstr(body, "descricao");
    errIfNull(descricaoStart);
    descricaoStart = strstr(descricaoStart, ":");
    errIfNull(descricaoStart);
    descricaoStart = strstr(descricaoStart, "\"");
    errIfNull(descricaoStart);
    descricaoStart++;

    char* descricaoEnd = strstr(descricaoStart, "\"");
    errIfNull(descricaoEnd);

    char buff[DESCRIPTION_SIZE];
    int length = descricaoEnd - descricaoStart;
    strncpy(buff, descricaoStart, length);
    buff[length] = '\0';
    strcpy(transaction->descricao, buff);

    getCurrentTime(transaction->realizada_em);

    return 0;
}

// Serialize post transaction response
void setPostResponse(User* user, char* responseBuffer) {
    const char* postResponseTemplate = "HTTP/1.1 200 OK\nContent-Type: application/json\n\n{\"limite\":%d, \"saldo\":%d}";
    sprintf(responseBuffer, postResponseTemplate, user->limit, user->total);
}

// Serialize get extract response
void setGetResponse(User* user, char* responseBuffer) {
    char body[RESPONSE_BODY_SIZE] = "";
    char transactionData[16 * 1024];
    char dateTime[DATE_SIZE];

    getCurrentTime(dateTime);
    const char* userDataTemplate = "{\"saldo\":%d,\"data_extrato\":\"%s\",\"limite\":%d,\"ultimas_transacoes\":[";
    sprintf(transactionData,
            userDataTemplate,
            user->total, dateTime, user->limit);
    strcat(body, transactionData);

    Transaction orderedTransactions[10];
    getOrderedTransactions(user, orderedTransactions);

    const char* transactionTemplate = "{\"valor\":%d,\"tipo\":\"%c\",\"descricao\":\"%s\",\"realizada_em\":\"%s\"}";
    for (int i = 0; i < user->nTransactions; i++) {
        Transaction transaction = orderedTransactions[i];

        sprintf(transactionData,
                transactionTemplate,
                transaction.valor, transaction.tipo, transaction.descricao, transaction.realizada_em);

        strcat(body, transactionData);

        if (i < user->nTransactions - 1) {
            strcat(body, ",");
        }
    }
    strcat(body, "]}");

    sprintf(responseBuffer, successResponseJsonTemplate, body);
#ifdef LOGGING
    printf("response: %s\n", responseBuffer);
#endif
}

int handlePostRequest(int clientSocket, char* buffer, int bufferSize) {
    // get id from request path
    int id = getIdFromPOSTRequest(buffer, bufferSize);
    if (id == -1) {
        return NOT_FOUND(clientSocket);
    }

    Transaction transaction;
    int parseResult = getTransactionFromPOSTRequest(buffer, bufferSize, &transaction);
    if (parseResult == -1) {
        return BAD_REQUEST(clientSocket);
    }

    User user;
    // update user on db by id
    int transactionResult = updateUserWithTransaction(id, &transaction, &user);

    if (transactionResult == TRANSACTION_ERROR) {
        return UNPROCESSABLE_ENTITY(clientSocket);
    }

    // serialize user to response
    char response[RESPONSE_SIZE];
    setPostResponse(&user, response);

    // send response
    return OK(clientSocket, response);
}

int handleGetRequest(int clientSocket, char* buffer, int bufferSize) {
    // get id from request path
    int id = getIdFromGETRequest(buffer, bufferSize);
    if (id == -1) {
        return NOT_FOUND(clientSocket);
    }

    // get user from db by id
    User user;
    readUser(&user, id);

    // serialize user to response
    char response[RESPONSE_SIZE];
    setGetResponse(&user, response);

    // send response
    return OK(clientSocket, response);
}

int handleRequest(char* buffer, int bytesRead, int clientSocket) {
    buffer[bytesRead] = '\0';
#ifdef LOGGING
    const char separator[] = "\n----------------------------------------------\n";
    printf("Received:");
    puts(separator);
    printf("%s\n", buffer);
    puts(separator);
    printf("(%d bytes read)\n\n", bytesRead);
#endif

    // Method alone has 3 bytes, so we need at least 4 bytes to consume a request
    if (bytesRead < 4) {
        return BAD_REQUEST(clientSocket);
    }

    if (partialEqual(buffer, GET_METHOD, GET_METHOD_LENGTH)) {
        return handleGetRequest(clientSocket, buffer, bytesRead);
    } else if (partialEqual(buffer, POST_METHOD, POST_METHOD_LENGTH)) {
        return handlePostRequest(clientSocket, buffer, bytesRead);
    }
    return BAD_REQUEST(clientSocket);
}

int main() {
    initDb();

    int serverSocket = setupServer(SERVER_PORT, SERVER_BACKLOG);
#ifdef LOGGING
    printf("Server is running(%d)\n", serverSocket);
    printf("Listening on port %d\n", SERVER_PORT);
    printf("FD_SETSIZE: %d\n\n", FD_SETSIZE);
#endif

    fd_set currentSockets, readySockets;

    FD_ZERO(&currentSockets);
    FD_SET(serverSocket, &currentSockets);

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

                    if (bytesRead >= 1 && bytesRead < SOCKET_READ_SIZE) {
                        handleRequest(buffer, bytesRead, i);
#ifdef LOGGING
                        printf("Request handled\n");
#endif
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