#pragma once
#ifndef TRANSACTION_H
#define TRANSACTION_H

typedef struct Transaction {
    int id;
    int type;
    double amount;
    char time[30];
    char description[200];
    struct Transaction* next;
} Transaction;

typedef struct PersonnelReport {
    char doctor_id[20];
    char doctor_name[20];
    char department[30];
    int count;
    struct PersonnelReport* next;
} PersonnelReport;

#define TRANS_OUTPATIENT_INCOME      1
#define TRANS_INPATIENT_BED_INCOME   2
#define TRANS_DRUG_INCOME            3
#define TRANS_EXAM_INCOME            4
#define TRANS_RECHARGE               5
#define TRANS_INPATIENT_DEPOSIT      6
#define TRANS_INPATIENT_SUPPLEMENT   7
#define TRANS_INPATIENT_REFUND       8

extern Transaction* transactionList;
extern PersonnelReport* personnelReportList;

void reportMenu(void);
void parttimereport(char* start, char* end);
int appendTransactionSafe(int type, double amount, const char* description);
void appendTransaction(int type, double amount, const char* description);

#endif
