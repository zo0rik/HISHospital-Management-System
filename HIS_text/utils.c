#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "models.h"
#include "utils.h"
#include <conio.h>

// 密码输入：显示 *
void safeprintPassword(char pwd[], int maxLen) {
    int i = 0;
    char ch;
    while (1) {
        ch = _getch(); 
        if (ch == '\r') break;   // 回车结束
        // 退格键处理
        if (ch == '\b') {
            if (i > 0) {
                i--;
                printf("\b \b");
            }
            continue;
        }
        if (i < maxLen - 1) {
            pwd[i++] = ch;
            printf("*");        // 只输出 *
        }
    }
    pwd[i] = '\0';
    printf("\n");
}


void safeGetString(char* buffer, int size) {
    int i;

    if (fgets(buffer, size, stdin) != NULL) {
        if (strchr(buffer, '\n') == NULL) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }
        else {
            buffer[strcspn(buffer, "\n")] = '\0';
        }

        for (i = 0; buffer[i] != '\0'; i++) {
            if (buffer[i] == ' ') {
                buffer[i] = '_';
            }
        }
    }
    else {
        buffer[0] = '\0';
        clearerr(stdin);
    }
}

int safeGetInt() {
    char buffer[100];
    int value;
    char extra;
    while (1) {
        value = 0;
        safeGetString(buffer, sizeof(buffer));
        if (strlen(buffer) == 0) {
            return 0;
        }
        if (sscanf(buffer, "%d%c", &value, &extra) == 1) {
            return value;
        }
        printf("  [!] 输入格式不合法，请重新输入一个有效的整数 (输入0返回): ");
    }
}

double safeGetDouble() {
    char buffer[100];
    double value;
    char extra;
    while (1) {
        value = 0.0;
        safeGetString(buffer, sizeof(buffer));
        if (strlen(buffer) == 0) return 0.0;
        if (strcmp(buffer, "0") == 0 || strcmp(buffer, "0.0") == 0) return 0.0;
        if (sscanf(buffer, "%lf%c", &value, &extra) == 1) {
            if (value < 0) {
                printf("  [!] 金额不能为负数，请重新输入 (输入0取消): ");
                continue;
            }
            return value;
        }
        printf("  [!] 输入格式不合法，请输入有效的金额数值 (输入0取消): ");
    }
}

int safeGetPositiveInt() {
    int val;
    while (1) {
        val = safeGetInt();
        if (val > 0) return val;
        if (val == 0) return 0;
        printf("  [!] 数值不能为负数，请重新输入有效正整数 (输入0返回): ");
    }
}

void safeGetGender(char* buffer, int size) {
    while (1) {
        safeGetString(buffer, size);
        if (strcmp(buffer, "0") == 0) return;
        if (strcmp(buffer, "男性") == 0 || strcmp(buffer, "女性") == 0) {
            return;
        }
        if (strcmp(buffer, "男") == 0) { strcpy(buffer, "男性"); return; }
        if (strcmp(buffer, "女") == 0) { strcpy(buffer, "女性"); return; }
        if (strlen(buffer) == 0) continue;
        printf("  [!] 性别信息只能填入【男】或【女】，请重新输入 (输入0取消): ");
    }
}

/* 新增：严格的密码规则约束器 */
void safeGetPassword(char* buffer, int size) {
    int i, valid;
    while (1) {
        safeGetString(buffer, size);
        if (strcmp(buffer, "0") == 0) return;
        if (strlen(buffer) == 0) continue;

        valid = 1;
        for (i = 0; buffer[i] != '\0'; i++) {
            if (!((buffer[i] >= '0' && buffer[i] <= '9') ||
                (buffer[i] >= 'a' && buffer[i] <= 'z') ||
                (buffer[i] >= 'A' && buffer[i] <= 'Z'))) {
                valid = 0;
                break;
            }
        }

        if (valid) return;
        printf("  [!] 非法输入：密码只能由【数字】和【字母】组合，不能包含特殊字符，请重输 (输入0取消): ");
    }
}

void getCurrentTimeStr(char* buffer, int size) {
    time_t t;
    struct tm* tm_info;
    t = time(NULL);
    tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d_%H:%M:%S", tm_info);
}

void loadAllDataFromTxt() {
    FILE* fp;

    // ---------------------------------------------------------
    // 1. 加载患者信息 (Patients)
    // ---------------------------------------------------------
    fp = fopen("patients.txt", "r");
    if (fp) {
        char line[1024];
        Patient* p_tail = patientHead;
        while (p_tail->next != NULL) p_tail = p_tail->next; // 寻找真实尾节点

        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = 0; // 去除行末换行符
            if (strlen(line) == 0) continue; // 跳过空行

            Patient p_temp = { 0 }; // 初始化清空
            char* token = strtok(line, ",");
            if (token) strcpy(p_temp.id, token);
            token = strtok(NULL, ","); if (token) strcpy(p_temp.password, token);
            token = strtok(NULL, ","); if (token) strcpy(p_temp.name, token);
            token = strtok(NULL, ","); if (token) strcpy(p_temp.gender, token);
            token = strtok(NULL, ","); if (token) p_temp.age = atoi(token);
            token = strtok(NULL, ","); if (token) strcpy(p_temp.allergy, token);
            token = strtok(NULL, ","); if (token) p_temp.isEmergency = atoi(token);
            token = strtok(NULL, ","); if (token) p_temp.balance = atof(token);

            Patient* p_node = (Patient*)malloc(sizeof(Patient));
            *p_node = p_temp;
            p_node->next = NULL;
            p_tail->next = p_node;
            p_tail = p_node;
        }
        fclose(fp);
    }

    // ---------------------------------------------------------
    // 2. 加载药品信息 (Medicines)
    // ---------------------------------------------------------
    fp = fopen("medicines.txt", "r");
    if (fp) {
        char line[512];
        Medicine* m_tail = medicineHead;
        while (m_tail->next != NULL) m_tail = m_tail->next;

        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = 0;
            if (strlen(line) == 0) continue;

            Medicine m_temp = { 0 };
            char* token = strtok(line, ",");
            if (token) strcpy(m_temp.id, token);
            token = strtok(NULL, ","); if (token) strcpy(m_temp.name, token);
            token = strtok(NULL, ","); if (token) m_temp.stock = atoi(token);
            token = strtok(NULL, ","); if (token) m_temp.price = atof(token);
            token = strtok(NULL, ","); if (token) strcpy(m_temp.expiryDate, token);

            Medicine* m_node = (Medicine*)malloc(sizeof(Medicine));
            *m_node = m_temp;
            m_node->next = NULL;
            m_tail->next = m_node;
            m_tail = m_node;
        }
        fclose(fp);
    }

    // ---------------------------------------------------------
    // 3. 加载医护信息 (Staff)
    // ---------------------------------------------------------
    fp = fopen("staff.txt", "r");
    if (fp) {
        char line[512];
        Staff* s_tail = staffHead;
        while (s_tail->next != NULL) s_tail = s_tail->next;

        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = 0;
            if (strlen(line) == 0) continue;

            Staff s_temp = { 0 };
            char* token = strtok(line, ",");
            if (token) strcpy(s_temp.id, token);
            token = strtok(NULL, ","); if (token) strcpy(s_temp.password, token);
            token = strtok(NULL, ","); if (token) strcpy(s_temp.name, token);
            token = strtok(NULL, ","); if (token) strcpy(s_temp.department, token);
            token = strtok(NULL, ","); if (token) strcpy(s_temp.level, token);

            Staff* s_node = (Staff*)malloc(sizeof(Staff));
            *s_node = s_temp;
            s_node->next = NULL;
            s_tail->next = s_node;
            s_tail = s_node;
        }
        fclose(fp);
    }

    // ---------------------------------------------------------
    // 4. 加载业务流水记录 (Records)
    // ---------------------------------------------------------
    fp = fopen("records.txt", "r");
    if (fp) {
        char line[1024];
        Record* r_tail = recordHead;
        while (r_tail->next != NULL) r_tail = r_tail->next;

        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = 0;
            if (strlen(line) == 0) continue;

            Record r_temp = { 0 };
            char* token = strtok(line, ",");
            if (token) strcpy(r_temp.recordId, token);
            token = strtok(NULL, ","); if (token) r_temp.type = atoi(token);
            token = strtok(NULL, ","); if (token) strcpy(r_temp.patientId, token);
            token = strtok(NULL, ","); if (token) strcpy(r_temp.staffId, token);
            token = strtok(NULL, ","); if (token) r_temp.cost = atof(token);
            token = strtok(NULL, ","); if (token) r_temp.isPaid = atoi(token);
            token = strtok(NULL, ","); if (token) strcpy(r_temp.description, token);
            token = strtok(NULL, ","); if (token) strcpy(r_temp.createTime, token);

            Record* r_node = (Record*)malloc(sizeof(Record));
            *r_node = r_temp;
            r_node->next = NULL;
            r_tail->next = r_node;
            r_tail = r_node;
        }
        fclose(fp);
    }

    // ---------------------------------------------------------
    // 5. 加载病床信息 (Beds)
    // ---------------------------------------------------------
    fp = fopen("beds.txt", "r");
    if (fp) {
        char line[512];
        Bed* b_tail = bedHead;
        while (b_tail->next != NULL) b_tail = b_tail->next;

        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = 0;
            if (strlen(line) == 0) continue;

            Bed b_temp = { 0 };
            char* token = strtok(line, ",");
            if (token) strcpy(b_temp.bedId, token);
            token = strtok(NULL, ","); if (token) b_temp.isOccupied = atoi(token);
            token = strtok(NULL, ","); if (token) strcpy(b_temp.patientId, token);
            token = strtok(NULL, ","); if (token) strcpy(b_temp.wardType, token);
            token = strtok(NULL, ","); if (token) strcpy(b_temp.bedType, token);
            token = strtok(NULL, ","); if (token) b_temp.price = atof(token);
            token = strtok(NULL, ","); if (token) b_temp.isRoundsDone = atoi(token);

            Bed* b_node = (Bed*)malloc(sizeof(Bed));
            *b_node = b_temp;
            b_node->next = NULL;
            b_tail->next = b_node;
            b_tail = b_node;
        }
        fclose(fp);
    }
}

void saveAllDataToTxt() {
    FILE* fp;

    // ---------------------------------------------------------
    // 1. 保存患者信息 (Patients)
    // ---------------------------------------------------------
    fp = fopen("patients.txt", "w");
    if (fp) {
        Patient* p = patientHead->next;
        while (p) {
            // 【修改点】：全部采用企业级 CSV 的逗号分隔符格式
            fprintf(fp, "%s,%s,%s,%s,%d,%s,%d,%.2f\n",
                p->id, p->password, p->name, p->gender,
                p->age, p->allergy, p->isEmergency, p->balance);
            p = p->next;
        }
        fclose(fp);
    }

    // ---------------------------------------------------------
    // 2. 保存药品信息 (Medicines)
    // ---------------------------------------------------------
    fp = fopen("medicines.txt", "w");
    if (fp) {
        Medicine* m = medicineHead->next;
        while (m) {
            fprintf(fp, "%s,%s,%d,%.2f,%s\n",
                m->id, m->name, m->stock, m->price, m->expiryDate);
            m = m->next;
        }
        fclose(fp);
    }

    // ---------------------------------------------------------
    // 3. 保存医护信息 (Staff)
    // ---------------------------------------------------------
    fp = fopen("staff.txt", "w");
    if (fp) {
        Staff* s = staffHead->next;
        while (s) {
            fprintf(fp, "%s,%s,%s,%s,%s\n",
                s->id, s->password, s->name, s->department, s->level);
            s = s->next;
        }
        fclose(fp);
    }

    // ---------------------------------------------------------
    // 4. 保存业务流水记录 (Records)
    // ---------------------------------------------------------
    fp = fopen("records.txt", "w");
    if (fp) {
        Record* r = recordHead->next;
        while (r) {
            fprintf(fp, "%s,%d,%s,%s,%.2f,%d,%s,%s\n",
                r->recordId, r->type, r->patientId, r->staffId,
                r->cost, r->isPaid, r->description, r->createTime);
            r = r->next;
        }
        fclose(fp);
    }

    // ---------------------------------------------------------
    // 5. 保存病床信息 (Beds)
    // ---------------------------------------------------------
    fp = fopen("beds.txt", "w");
    if (fp) {
        Bed* b = bedHead->next;
        while (b) {
            // 【核心防崩保护】：如果病床为空（没有患者ID），必须填入占位符"无"
            // 防止生成连续逗号 ",," 导致下次读取时 strtok 错位！
            char safePatientId[30];
            if (strlen(b->patientId) == 0) {
                strcpy(safePatientId, "无");
            }
            else {
                strcpy(safePatientId, b->patientId);
            }

            fprintf(fp, "%s,%d,%s,%s,%s,%.2f,%d\n",
                b->bedId, b->isOccupied, safePatientId,
                b->wardType, b->bedType, b->price, b->isRoundsDone);
            b = b->next;
        }
        fclose(fp);
    }
}