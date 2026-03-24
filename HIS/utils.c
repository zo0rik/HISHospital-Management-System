#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "models.h"
#include "utils.h"

// ---------------------------------------------------------
// 1. 安全输入工具：读取字符串并自动消除换行符
// ---------------------------------------------------------
void safeGetString(char* buffer, int size) {
    if (fgets(buffer, size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0'; // 将末尾的换行符替换为字符串结束符
    }
    else {
        buffer[0] = '\0';
    }
}

// ---------------------------------------------------------
// 2. 安全输入工具：读取整数 (防止用户输入字母导致程序崩溃)
// ---------------------------------------------------------
int safeGetInt() {
    char buffer[100];
    safeGetString(buffer, sizeof(buffer));
    int value = 0;
    sscanf(buffer, "%d", &value);
    return value;
}

// ---------------------------------------------------------
// 3. 安全输入工具：读取浮点数 (用于金额)
// ---------------------------------------------------------
double safeGetDouble() {
    char buffer[100];
    safeGetString(buffer, sizeof(buffer));
    double value = 0.0;
    sscanf(buffer, "%lf", &value);
    return value;
}

// ---------------------------------------------------------
// 4. 从本地 TXT 文件加载所有基础数据到内存链表
// ---------------------------------------------------------
void loadAllDataFromTxt() {
    // [加载患者档案] - 包含新加的密码字段(共8个字段)
    FILE* fp = fopen("patients.txt", "r");
    if (fp) {
        Patient* tail = patientHead;
        Patient temp;
        while (fscanf(fp, "%19s %49s %99s %99s %d %99s %d %lf",
            temp.id, temp.password, temp.name, temp.gender, &temp.age, temp.allergy, &temp.isEmergency, &temp.balance) == 8) {
            Patient* newNode = (Patient*)malloc(sizeof(Patient));
            *newNode = temp;
            newNode->next = NULL;
            tail->next = newNode;
            tail = newNode;
        }
        fclose(fp);
    }

    // [加载药品库存]
    fp = fopen("medicines.txt", "r");
    if (fp) {
        Medicine* tail = medicineHead;
        Medicine temp;
        while (fscanf(fp, "%19s %99s %d %lf %99s",
            temp.id, temp.name, &temp.stock, &temp.price, temp.expiryDate) == 5) {
            Medicine* newNode = (Medicine*)malloc(sizeof(Medicine));
            *newNode = temp;
            newNode->next = NULL;
            tail->next = newNode;
            tail = newNode;
        }
        fclose(fp);
    }

    // [加载医护人员名单]
    fp = fopen("staff.txt", "r");
    if (fp) {
        Staff* tail = staffHead;
        Staff temp;
        while (fscanf(fp, "%19s %49s %99s %99s %99s",
            temp.id, temp.password, temp.name, temp.department, temp.level) == 5) {
            Staff* newNode = (Staff*)malloc(sizeof(Staff));
            *newNode = temp;
            newNode->next = NULL;
            tail->next = newNode;
            tail = newNode;
        }
        fclose(fp);
    }

    // [加载全院流水账单]
    fp = fopen("records.txt", "r");
    if (fp) {
        Record* tail = recordHead;
        Record temp;
        while (fscanf(fp, "%29s %d %19s %19s %lf %d %199s",
            temp.recordId, &temp.type, temp.patientId, temp.staffId, &temp.cost, &temp.isPaid, temp.description) == 7) {
            Record* newNode = (Record*)malloc(sizeof(Record));
            *newNode = temp;
            newNode->next = NULL;
            tail->next = newNode;
            tail = newNode;
        }
        fclose(fp);
    }
}

// ---------------------------------------------------------
// 5. 将内存链表中的所有数据同步保存到本地 TXT 文件
// ---------------------------------------------------------
void saveAllDataToTxt() {
    FILE* fp = fopen("patients.txt", "w");
    if (fp) {
        Patient* p = patientHead->next;
        while (p) {
            fprintf(fp, "%s %s %s %s %d %s %d %.2f\n",
                p->id, p->password, p->name, p->gender, p->age, p->allergy, p->isEmergency, p->balance);
            p = p->next;
        }
        fclose(fp);
    }

    fp = fopen("medicines.txt", "w");
    if (fp) {
        Medicine* m = medicineHead->next;
        while (m) {
            fprintf(fp, "%s %s %d %.2f %s\n", m->id, m->name, m->stock, m->price, m->expiryDate);
            m = m->next;
        }
        fclose(fp);
    }

    fp = fopen("staff.txt", "w");
    if (fp) {
        Staff* s = staffHead->next;
        while (s) {
            fprintf(fp, "%s %s %s %s %s\n", s->id, s->password, s->name, s->department, s->level);
            s = s->next;
        }
        fclose(fp);
    }

    fp = fopen("records.txt", "w");
    if (fp) {
        Record* r = recordHead->next;
        while (r) {
            fprintf(fp, "%s %d %s %s %.2f %d %s\n",
                r->recordId, r->type, r->patientId, r->staffId, r->cost, r->isPaid, r->description);
            r = r->next;
        }
        fclose(fp);
    }
}