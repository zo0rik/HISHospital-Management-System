#pragma once
#ifndef MODELS_H
#define MODELS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Patient {
    char id[20];
    char password[50];
    char name[100];
    char gender[10];
    int age;
    char allergy[100];
    int isEmergency;
    double balance;
    struct Patient* next;
} Patient, * PatientList;

// 【修改点】：统一使用 Staff 结构体管理所有医生与职工
typedef struct Staff {
    char id[20];
    char password[50];
    char name[100];
    char department[100];
    char level[100]; // 对应医生的“职称”
    char sex[10];    // 【新增】性别字段
    struct Staff* next;
} Staff, * StaffList;

typedef struct Medicine {
    char id[20];
    char name[100];
    int stock;
    double price;
    char expiryDate[20];
    struct Medicine* next;
} Medicine, * MedicineList;

typedef struct Record {
    char recordId[30];
    int type;
    char patientId[20];
    char staffId[20];
    double cost;
    int isPaid;
    char description[300];
    char createTime[30];
    struct Record* next;
} Record, * RecordList;

typedef struct Bed {
    char bedId[30];
    int isOccupied;
    char patientId[20];
    char wardType[50];
    char bedType[50];
    double price;
    int isRoundsDone;
    struct Bed* next;
} Bed, * BedList;

extern PatientList patientHead;
extern StaffList staffHead;
extern MedicineList medicineHead;
extern RecordList recordHead;
extern BedList bedHead;

#endif