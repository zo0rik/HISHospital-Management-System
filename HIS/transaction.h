#ifndef TRANSACTION_H
#define TRANSACTION_H

typedef struct Transaction {
    int id;
    int type;       // 1=门诊收入, 2=住院收入, 3=药品收入
    float amount;
    char time[30];
    char description[100];
    struct Transaction* next;
} Transaction;

extern Transaction* transactionList;

void loadTransactions(void);
void saveTransactions(void);
void reportMenu(void);

#endif
