#pragma once
#ifndef MODELS_H
#define MODELS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// ==========================================
// 1. 系统统一登录账号节点 (融合基础认证信息)
// ==========================================
typedef struct User {
    char id[20];            // 账号/工号
    char password[50];      // 密码
    char role[20];          // 角色: admin/doctor/nurse/patient
    char name[100];         // 姓名
    char department[50];    // 所属科室 (患者可为空或"无")
    struct User* next;
} User, * UserList;

// ==========================================
// 2. 患者档案链表节点 (详细医疗与财务信息)
// ==========================================
typedef struct Patient {
    char id[20];            // 唯一患者ID (如 P20251000)
    char name[100];         // 姓名
    char gender[10];        // 性别
    int age;                // 年龄 (-1表示急诊未录入)
    char allergy[100];      // 过敏史
    int isEmergency;        // 是否急诊 (1:是 0:否)
    double balance;         // 账户余额
    struct Patient* next;   // 指向下一个患者节点的指针
} Patient, * PatientList;

// ==========================================
// 3. 医护人员链表节点 (详细人事信息)
// ==========================================
typedef struct Staff {
    char id[20];            // 工号 (如 D101)
    char password[50];      // 登录密码
    char name[100];         // 姓名
    char department[100];   // 所属科室 (如 内科、外科)
    char level[100];        // 医生级别 (如 主任医师、住院医师)
    struct Staff* next;
} Staff, * StaffList;

// ==========================================
// 4. 药品库存链表节点 (融合了通用名和科室限制)
// ==========================================
typedef struct Medicine {
    char id[20];            // 药品编号
    char name[100];         // 药品名称 (商品名)
    char commonName[100];   // 药品通用名 (从V2引入)
    int stock;              // 当前库存数量
    double price;           // 药品单价
    char department[50];    // 关联科室 (从V2引入)
    char expiryDate[20];    // 有效期 (如 2026-12-31)
    struct Medicine* next;
} Medicine, * MedicineList;

// ==========================================
// 5. 医生排班链表节点 (完整从V2引入并规范化)
// ==========================================
typedef struct Schedule {
    char docId[20];         // 医生工号
    char name[50];          // 医生姓名
    char department[50];    // 所属科室
    char date[20];          // 出诊日期 (如 2026-03-24)
    char timeSlot[20];      // 出诊时段 (上午/下午/夜班)
    char status[20];        // 状态 (在职/请假/离职/满员)
    struct Schedule* next;
} Schedule, * ScheduleList;

// ==========================================
// 6. 病房/病床链表节点 (融合了房间号、类型、单价)
// ==========================================
typedef struct Bed {
    char bedId[20];         // 病房ID (如 R101, 从V2引入)
    int bedNumber;          // 床位号 (如 1, 2)
    char department[50];    // 所属科室 (从V2引入)
    char type[20];          // 病房类型 (单人/双人/三人/陪护, 从V2引入)
    int isOccupied;         // 占用状态：1:占用 0:空闲
    double price;           // 每日床位价格 (从V2引入)
    char patientId[20];     // 当前占用该床位的患者ID (空闲时为"none"或空)
    struct Bed* next;
} Bed, * BedList;

// ==========================================
// 7. 诊疗/挂号/缴费流水记录链表 (融合全要素)
// ==========================================
typedef struct Record {
    char recordId[30];      // 记录流水号 (如 R20252000)
    int type;               // 记录分类码：1:挂号 2:看诊处方 3:检查 4:住院
    char typeName[50];      // 记录类型名 (如 "挂号记录", 从V2引入)
    char patientId[20];     // 关联的患者ID
    char staffId[20];       // 关联的经办医生ID (可为空)
    char description[200];  // 诊断描述、检查项目或处方详情 (同V2 detail)
    double cost;            // 产生费用 (同V2 amount)
    int isPaid;             // 状态标记：0:未缴费 1:已缴费 2:已就诊/已处理
    struct Record* next;
} Record, * RecordList;

// ==========================================
// 全局头指针声明 (在 main.c 中进行内存分配初始化)
// ==========================================
extern UserList userHead;           // 系统登录用户链表头
extern PatientList patientHead;     // 患者档案链表头
extern StaffList staffHead;         // 医护人员链表头
extern MedicineList medicineHead;   // 药房库存链表头
extern ScheduleList scheduleHead;   // 排班信息链表头
extern BedList bedHead;             // 病床管理链表头
extern RecordList recordHead;       // 全院流水账单链表头

#endif