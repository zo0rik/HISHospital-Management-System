#pragma once
#ifndef MODELS_H
#define MODELS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==========================================
// 1. 患者档案链表节点
// ==========================================
typedef struct Patient {
    char id[20];            // 唯一患者ID (系统自动生成，如 P20251000)
    char password[50];      // 登录密码 (用于在主界面进行身份验证)
    char name[100];         // 姓名
    char gender[10];        // 性别
    int age;                // 年龄 (-1表示急诊未录入)
    char allergy[100];      // 过敏史
    int isEmergency;        // 是否急诊 (1:是 0:否)
    double balance;         // 账户余额 (用于门诊缴费、住院押金扣款)
    struct Patient* next;   // 指向下一个患者节点的指针
} Patient, * PatientList;

// ==========================================
// 2. 医护人员链表节点
// ==========================================
typedef struct Staff {
    char id[20];            // 工号 (用于主界面登录，如 D101)
    char password[50];      // 登录密码
    char name[100];         // 姓名
    char department[100];   // 所属科室 (如 呼吸内科、外科)
    char level[100];        // 医生级别 (如 主任医师、住院医师，影响挂号费)
    struct Staff* next;     // 指向下一个医护节点的指针
} Staff, * StaffList;

// ==========================================
// 3. 药品库存链表节点
// ==========================================
typedef struct Medicine {
    char id[20];            // 药品编号
    char name[100];         // 药品名称 (商品名/通用名)
    int stock;              // 当前物理库存数量
    double price;           // 药品单价
    char expiryDate[20];    // 有效期 (如 2026-12-31)
    struct Medicine* next;  // 指向下一个药品节点的指针
} Medicine, * MedicineList;

// ==========================================
// 4. 全院业务流水记录链表 (核心纽带)
// ==========================================
typedef struct Record {
    char recordId[30];      // 记录流水号 (如 REG20255000)
    int type;               // 记录分类码：1:挂号 2:看诊/处方 3:检查单 4:住院押金
    char patientId[20];     // 关联的患者ID (外键)
    char staffId[20];       // 关联的经办医生工号 (外键)
    double cost;            // 产生费用金额
    int isPaid;             // 状态标记：0:待缴费 1:已缴费 2:已就诊/已处理
    char description[200];  // 描述详情 (如 "处方药:阿莫西林 x 2")
    struct Record* next;    // 指向下一条记录的指针
} Record, * RecordList;

// ==========================================
// 5. 病房与床位链表节点
// ==========================================
typedef struct Bed {
    int bedNumber;          // 床位号 (如 101, 102)
    int isOccupied;         // 占用状态：1:已占用 0:空闲
    char patientId[20];     // 当前占用该床位的患者ID (空闲时为空字符串)
    struct Bed* next;       // 指向下一个床位节点的指针
} Bed, * BedList;

// ==========================================
// 全局头指针声明 (在 main.c 中进行内存分配和初始化)
// ==========================================
extern PatientList patientHead;
extern StaffList staffHead;
extern MedicineList medicineHead;
extern RecordList recordHead;
extern BedList bedHead;

#endif