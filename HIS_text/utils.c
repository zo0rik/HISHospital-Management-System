#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "models.h"
#include "utils.h"

void safeGetString(char* buffer, int size) {
    if (fgets(buffer, size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
    }
    else { buffer[0] = '\0'; }
}

int safeGetInt() {
    char buffer[100]; safeGetString(buffer, sizeof(buffer));
    int value = 0; sscanf(buffer, "%d", &value);
    return value;
}

double safeGetDouble() {
    char buffer[100]; safeGetString(buffer, sizeof(buffer));
    double value = 0.0; sscanf(buffer, "%lf", &value);
    return value;
}

// 【防呆设计】强制要求输入大于0的正整数
int safeGetPositiveInt() {
    while (1) {
        int val = safeGetInt();
        if (val > 0) return val;
        printf("【输入错误】请输入大于0的有效整数: ");
    }
}

// 【防呆设计】强制限制性别输入
void safeGetGender(char* buffer, int size) {
    while (1) {
        safeGetString(buffer, size);
        if (strcmp(buffer, "男性") == 0 || strcmp(buffer, "女性") == 0) {
            return;
        }
        printf("【输入错误】只能填入 男性 或 女性，请重新输入: ");
    }
}

// 获取格式化系统时间（无空格，防止文件读取断裂）
void getCurrentTimeStr(char* buffer, int size) {
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d_%H:%M:%S", tm_info);
}

// ---------------------------------------------------------
// 数据读取模块：从本地 TXT 文件加载所有基础数据到内存链表
// ---------------------------------------------------------
void loadAllDataFromTxt() {
    // 1. [加载患者档案] - 8个字段
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

    // 2. [加载药品库存] - 5个字段
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

    // 3. [加载医护人员名单] - 5个字段
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

    // 4. [加载全院流水账单] - 8个字段 (包含最后的时间戳)
    fp = fopen("records.txt", "r");
    if (fp) {
        Record* tail = recordHead;
        Record temp;
        while (fscanf(fp, "%29s %d %19s %19s %lf %d %299s %29s",
            temp.recordId, &temp.type, temp.patientId, temp.staffId, &temp.cost, &temp.isPaid, temp.description, temp.createTime) == 8) {
            Record* newNode = (Record*)malloc(sizeof(Record));
            *newNode = temp;
            newNode->next = NULL;
            tail->next = newNode;
            tail = newNode;
        }
        fclose(fp);
    }

    // 5. [加载病床与查房数据] - 7个字段 (适配新版字符串 bedId 和 isRoundsDone)
    fp = fopen("beds.txt", "r");
    if (fp) {
        Bed* tail = bedHead;
        Bed temp;
        while (fscanf(fp, "%19s %d %19s %49s %49s %lf %d",
            temp.bedId, &temp.isOccupied, temp.patientId, temp.wardType, temp.bedType, &temp.price, &temp.isRoundsDone) == 7) {
            Bed* newNode = (Bed*)malloc(sizeof(Bed));
            *newNode = temp;
            newNode->next = NULL;
            tail->next = newNode;
            tail = newNode;
        }
        fclose(fp);
    }
}

// ---------------------------------------------------------
// 数据持久化模块：将内存链表中的所有数据同步保存到本地 TXT 文件
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
            fprintf(fp, "%s %d %s %s %.2f %d %s %s\n",
                r->recordId, r->type, r->patientId, r->staffId, r->cost, r->isPaid, r->description, r->createTime);
            r = r->next;
        }
        fclose(fp);
    }

    fp = fopen("beds.txt", "w");
    if (fp) {
        Bed* b = bedHead->next;
        while (b) {
            fprintf(fp, "%s %d %s %s %s %.2f %d\n",
                b->bedId, b->isOccupied, strlen(b->patientId) > 0 ? b->patientId : "none", b->wardType, b->bedType, b->price, b->isRoundsDone);
            b = b->next;
        }
        fclose(fp);
    }
}