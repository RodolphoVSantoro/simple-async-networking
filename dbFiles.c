#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_TRANSACTIONS 10
#define DATE_SIZE 32
#define DESCRIPTION_SIZE 32
#define FILE_NAME_SIZE 32
#define moveRight(index) (index = (index + 1) % MAX_TRANSACTIONS)

typedef struct TRANSACTION {
    int valor;
    char tipo;
    char descricao[DESCRIPTION_SIZE];
    char realizada_em[DATE_SIZE];
} Transaction;

typedef struct USER {
    int id;
    int limit, total;
    int nTransactions;
    int oldestTransaction;
    Transaction transactions[MAX_TRANSACTIONS];
} User;

void readUser(User* user, int id) {
    const char* template = "user%d.bin";
    char fname[FILE_NAME_SIZE];
    sprintf(fname, template, id);
    FILE* fpTotals = fopen(fname, "rb");
    fread(user, sizeof(User), 1, fpTotals);
    fclose(fpTotals);
}

void writeUser(User* user) {
    const char* template = "data/user%d.bin";
    char fname[FILE_NAME_SIZE];
    sprintf(fname, template, user->id);
    FILE* fpTotals = fopen(fname, "wb");
    fwrite(user, sizeof(User), 1, fpTotals);
    fclose(fpTotals);
}

void initDb() {
    const int limits[] = {100000, 80000, 1000000, 10000000, 500000};
    for (int i = 0; i < 5; i++) {
        User user;
        user.id = i + 1;
        user.limit = limits[i];
        user.total = 0;
        user.nTransactions = 0;
        user.oldestTransaction = 0;
        writeUser(&user);
    }
}

int addSaldo(User* user, Transaction* transaction) {
    if (transaction->tipo == 'c') {
        int newTotal = user->total - transaction->valor;
        if (-1 * newTotal > user->limit) {
            return -1;
        }
        user->total = newTotal;
        return 0;
    }
    user->total += transaction->valor;
    return 0;
}

// Returns -1 if the user has no limit
// Returns 0 if the transaction was successful
int addTransaction(User* user, Transaction* transaction) {
    int result_saldo = addSaldo(user, transaction);
    if (result_saldo != 0) {
        return -1;
    }
    if (user->nTransactions == 10) {
        user->transactions[user->oldestTransaction] = *transaction;
        moveRight(user->oldestTransaction);
        return 0;
    }

    user->transactions[user->nTransactions] = *transaction;
    user->nTransactions++;
    return 0;
}

void getOrderedTransactions(User* user, Transaction* transaction, Transaction* orderedTransactions) {
    int i = user->oldestTransaction;
    for (int j = 0; j < user->nTransactions; j++) {
        orderedTransactions[j] = user->transactions[i];
        moveRight(i);
    }
}

void addCurrentTime(Transaction* transaction) {
    time_t mytime = time(NULL);
    char* time_str = ctime(&mytime);
    time_str[strlen(time_str) - 1] = '\0';
    strcpy(transaction->realizada_em, time_str);
}

Transaction createTransaction(int valor, char tipo, char* descricao) {
    Transaction transaction;
    transaction.valor = valor;
    transaction.tipo = tipo;
    strcpy(transaction.descricao, descricao);
    addCurrentTime(&transaction);

    return transaction;
}

int main() {
    initDb();
    User user;
    readUser(&user, 3);
    printf("User 3 has %d\n", user.total);

    Transaction t1 = createTransaction(10000, 'c', "credito teste");
    int result = addTransaction(&user, &t1);
    if (result != 0) {
        printf("Transaction failed\n");
    }

    Transaction t2 = createTransaction(1500, 'd', "Deposito teste");
    result = addTransaction(&user, &t2);
    if (result != 0) {
        printf("Transaction failed\n");
    }

    writeUser(&user);
    readUser(&user, 3);
    printf("User 3 has %d\n", user.total);

    return 0;
}
