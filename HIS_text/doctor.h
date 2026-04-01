#ifndef DOCTOR_H
#define DOCTOR_H

typedef struct Doctor {
    int id;
    char name[50];
    char department[30];
    char title[20];
    char sex[10];
    struct Doctor* next;
} Doctor;

extern Doctor* doctorList;

void loadDoctors(void);
void saveDoctors(void);
void doctorMenu(void);

#endif