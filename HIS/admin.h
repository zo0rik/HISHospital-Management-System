#ifndef ADMIN_H
#define ADMIN_H

typedef struct Admin {
    char username[20];
    char password[20];
    char phone[11];
    char email[30];
} Admin;

extern Admin admin;

void loadAdminData(void);
void saveAdminData(void);
void changePassword(void);
void editPersonalInfo(void);
void personalMenu(void);

// 新增：向 main.c 暴露的管理端总入口
void adminMenu(void);

#endif