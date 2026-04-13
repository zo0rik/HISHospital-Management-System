#pragma once
#ifndef ADMIN_H
#define ADMIN_H

// 队友设计的 admin 结构体
typedef struct Admin {
    char username[20];
    char password[20];
	char phone[12];
    char email[30];
} Admin;

extern Admin admin;

// 数据管理及个人设置接口
void changePassword(void);
void editPersonalInfo(void);
void personalMenu(void);

// 向 main 暴露的管理端大屏入口
void adminMenu(void);

#endif
