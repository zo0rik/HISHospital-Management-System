#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "models.h"
#include "utils.h"

// ---------------------------------------------------------
// 基础工具：读取字符串并处理换行符
// ---------------------------------------------------------
// 使用 fgets 替代原生的 scanf，能够安全地读取包含空格的字符串，
// 并在读取后手动去除末尾的换行符，避免回车残留对后续输入造成影响。
void safeGetString(char* buffer, int size) {
    if (fgets(buffer, size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
    }
    else {
        buffer[0] = '\0';
    }
}

// ---------------------------------------------------------
// 安全输入模块：整数读取与容错处理
// ---------------------------------------------------------
// 解决原生 scanf 读取异常字符会导致输入流阻塞、控制台死循环的问题。
// 先将输入作为整行字符串读取，再利用 sscanf 的匹配返回值进行严格校验。
int safeGetInt() {
    char buffer[100];
    int value = 0;
    while (1) {
        safeGetString(buffer, sizeof(buffer));
        // 屏蔽用户直接敲击回车产生的空输入
        if (strlen(buffer) == 0) {
            continue;
        }
        char extra;
        // 仅当 sscanf 只成功匹配到一个整型数据时，才认为输入完全合法。
        // 如果输入类似 "12abc"，则会匹配到两个参数，从而触发拦截。
        if (sscanf(buffer, "%d%c", &value, &extra) == 1) {
            return value;
        }
        printf("  [!] 输入格式不合法，请重新输入一个有效的整数: ");
    }
}

// ---------------------------------------------------------
// 安全输入模块：浮点数读取与容错处理
// ---------------------------------------------------------
// 机制与 safeGetInt 相同，专门用于金额、价格等需要小数的业务场景。
double safeGetDouble() {
    char buffer[100];
    double value = 0.0;
    while (1) {
        safeGetString(buffer, sizeof(buffer));
        if (strlen(buffer) == 0) continue;

        char extra;
        if (sscanf(buffer, "%lf%c", &value, &extra) == 1) {
            return value;
        }
        printf("  [!] 输入格式不合法，请输入有效的金额数值: ");
    }
}

// ---------------------------------------------------------
// 业务约束：获取正整数
// ---------------------------------------------------------
// 强制要求返回值必须大于0，常用于诸如开药数量、住院天数等不能为负数或零的业务节点。
int safeGetPositiveInt() {
    while (1) {
        int val = safeGetInt();
        if (val > 0) return val;
        printf("  [!] 数值不能为零或负数，请重新输入有效正整数: ");
    }
}

// ---------------------------------------------------------
// 业务约束：获取规范化的性别字符串
// ---------------------------------------------------------
// 严格限制内容的录入，确保写入本地文件时字段统一，避免后续读取解析错位。
void safeGetGender(char* buffer, int size) {
    while (1) {
        safeGetString(buffer, size);
        if (strcmp(buffer, "男性") == 0 || strcmp(buffer, "女性") == 0) {
            return;
        }
        // 为了提升系统的输入体验，增加对简写的兼容转换
        if (strcmp(buffer, "男") == 0) { strcpy(buffer, "男性"); return; }
        if (strcmp(buffer, "女") == 0) { strcpy(buffer, "女性"); return; }

        printf("  [!] 性别信息只能填入【男】或【女】，请重新输入: ");
    }
}

// ---------------------------------------------------------
// 内部工具：生成连续格式的当前时间戳
// ---------------------------------------------------------
// 格式化当前系统时间。特意去除了日期与时间之间的空格（使用下划线代替），
// 这是为了防止 fscanf 在按空格读取本地 txt 数据时将时间戳错误地切割成两段。
void getCurrentTimeStr(char* buffer, int size) {
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d_%H:%M:%S", tm_info);
}

// ---------------------------------------------------------
// 数据加载引擎：从本地文件反序列化到内存链表
// ---------------------------------------------------------
// 系统启动时调用。采用尾插法构建各业务模块的单链表，
// 这样可以确保数据在内存中的顺序与本地文件中的记录顺序完全一致。
void loadAllDataFromTxt() {
    // 1. 加载患者档案信息
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

    // 2. 加载大药房物资库存
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

    // 3. 加载全院医护人员名录
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

    // 4. 加载全系统业务流水账单
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

    // 5. 加载住院部病床及查房状态
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
// 数据持久化引擎：将内存链表序列化回写至本地文件
// ---------------------------------------------------------
// 在系统安全退出时触发，遍历各个链表并覆写对应的 txt 文件，
// 确保全天所有增删改查的业务数据得到永久保存。
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