#pragma once
#ifndef DRUG_H
#define DRUG_H

#include "utils.h"

typedef struct Drug {
    int id;
    char name[50];
    int stock;
    double price;
    char batch[30];
    char expiry[30];
    char last_in[30];
    char last_out[30];
    struct Drug* next;
} Drug;

typedef struct DrugHistory {
    int drug_id;
    int type;
    int quantity;
    char time[30];  /* 【修复】原time[15]不够放"YYYY-MM-DD_HH:MM:SS"(20字节)，改为30 */
    struct DrugHistory* next;
} DrugHistory;

extern Drug* drugList;
extern DrugHistory* drugHistoryList;

void drugMenu(void);
void initDrugList();
Drug* findDrugById(int id);
Drug* findDrugByNameExact(const char* name);
int findDrugsByNameFuzzy(const char* keyword, Drug** results, int maxResults);

#endif