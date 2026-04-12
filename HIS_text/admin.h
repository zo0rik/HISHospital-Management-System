#pragma once
#ifndef ADMIN_H
#define ADMIN_H

typedef struct Admin {
    char username[20];
    char password[20];
    char phone[12];
    char email[30];
    struct Admin* next;
} Admin;

extern Admin admin;                 /* 当前已登录管理员的快照 */
extern Admin* adminHead;            /* 管理员链表头结点 */
extern Admin* currentAdminNode;     /* 当前已登录管理员节点 */

Admin* findAdminByCredentials(const char* username, const char* password);
void setCurrentAdminNode(Admin* node);
void freeAdminList(void);

// 数据管理及个人设置接口
void changePassword(void);
void editPersonalInfo(void);
void personalMenu(void);

// 向 main 暴露的管理端大屏入口
void adminMenu(void);

#endif
