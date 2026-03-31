#ifndef DRUG_H
#define DRUG_H

#include "utils.h"

typedef struct Drug {
    char id[15];
    char name[50];
    int stock;
    float price;
    char batch[30];
    char expiry[10];
    char last_in[30];
    char last_out[30];
    struct Drug* next;
} Drug;

typedef struct DrugHistory {
    char drug_id[15];
    int type;           // 1=Èë¿â, 2=³ö¿â
    int quantity;
    char time[30];
    struct DrugHistory* next;
} DrugHistory;

extern Drug* drugList;
extern DrugHistory* drugHistoryList;

void loadDrugs(void);
void saveDrugs(void);
void loadDrugHistory(void);
void saveDrugHistory(void);
void drugMenu(void);

#endif
#pragma once
