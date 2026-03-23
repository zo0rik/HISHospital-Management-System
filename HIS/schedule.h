#ifndef SCHEDULE_H
#define SCHEDULE_H

typedef struct Schedule {
    int doctor_id;
    char date[11];
    char shift[10];
    struct Schedule* next;
} Schedule;

extern Schedule* scheduleList;

void loadSchedules(void);
void saveSchedules(void);
void scheduleMenu(void);

#endif
