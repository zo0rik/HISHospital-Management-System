#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "models.h"
#include "utils.h"

void safeGetString(char* buffer, int size) {
    if (fgets(buffer, size, stdin) != NULL) {
        if (strchr(buffer, '\n') == NULL) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }
        else {
            buffer[strcspn(buffer, "\n")] = '\0';
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
        if (val == 0) return 0; // 支持输入0返回
        printf("  [!] 数值不能为负数，请重新输入有效正整数 (输入0返回): ");
    }
}

void safeGetGender(char* buffer, int size) {
    while (1) {
        safeGetString(buffer, size);
        if (strcmp(buffer, "0") == 0) return; // 支持输入0返回
        if (strcmp(buffer, "男性") == 0 || strcmp(buffer, "女性") == 0) {
            return;
        }
        if (strcmp(buffer, "男") == 0) { strcpy(buffer, "男性"); return; }
        if (strcmp(buffer, "女") == 0) { strcpy(buffer, "女性"); return; }
        if (strlen(buffer) == 0) continue;
        printf("  [!] 性别信息只能填入【男】或【女】，请重新输入 (输入0取消): ");
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
    Patient* p_tail; Patient p_temp; Patient* p_node;
    Medicine* m_tail; Medicine m_temp; Medicine* m_node;
    Staff* s_tail; Staff s_temp; Staff* s_node;
    Record* r_tail; Record r_temp; Record* r_node;
    Bed* b_tail; Bed b_temp; Bed* b_node;

    fp = fopen("patients.txt", "r");
    if (fp) {
        p_tail = patientHead;
        while (fscanf(fp, "%19s %49s %99s %99s %d %99s %d %lf",
            p_temp.id, p_temp.password, p_temp.name, p_temp.gender, &p_temp.age, p_temp.allergy, &p_temp.isEmergency, &p_temp.balance) == 8) {
            p_node = (Patient*)malloc(sizeof(Patient));
            *p_node = p_temp; p_node->next = NULL;
            p_tail->next = p_node; p_tail = p_node;
        }
        fclose(fp);
    }

    fp = fopen("medicines.txt", "r");
    if (fp) {
        m_tail = medicineHead;
        while (fscanf(fp, "%19s %99s %d %lf %99s",
            m_temp.id, m_temp.name, &m_temp.stock, &m_temp.price, m_temp.expiryDate) == 5) {
            m_node = (Medicine*)malloc(sizeof(Medicine));
            *m_node = m_temp; m_node->next = NULL;
            m_tail->next = m_node; m_tail = m_node;
        }
        fclose(fp);
    }

    fp = fopen("staff.txt", "r");
    if (fp) {
        s_tail = staffHead;
        while (fscanf(fp, "%19s %49s %99s %99s %99s",
            s_temp.id, s_temp.password, s_temp.name, s_temp.department, s_temp.level) == 5) {
            s_node = (Staff*)malloc(sizeof(Staff));
            *s_node = s_temp; s_node->next = NULL;
            s_tail->next = s_node; s_tail = s_node;
        }
        fclose(fp);
    }

    fp = fopen("records.txt", "r");
    if (fp) {
        r_tail = recordHead;
        while (fscanf(fp, "%29s %d %19s %19s %lf %d %299s %29s",
            r_temp.recordId, &r_temp.type, r_temp.patientId, r_temp.staffId, &r_temp.cost, &r_temp.isPaid, r_temp.description, r_temp.createTime) == 8) {
            r_node = (Record*)malloc(sizeof(Record));
            *r_node = r_temp; r_node->next = NULL;
            r_tail->next = r_node; r_tail = r_node;
        }
        fclose(fp);
    }

    fp = fopen("beds.txt", "r");
    if (fp) {
        b_tail = bedHead;
        while (fscanf(fp, "%19s %d %19s %49s %49s %lf %d",
            b_temp.bedId, &b_temp.isOccupied, b_temp.patientId, b_temp.wardType, b_temp.bedType, &b_temp.price, &b_temp.isRoundsDone) == 7) {
            b_node = (Bed*)malloc(sizeof(Bed));
            *b_node = b_temp; b_node->next = NULL;
            b_tail->next = b_node; b_tail = b_node;
        }
        fclose(fp);
    }
}

void saveAllDataToTxt() {
    FILE* fp;
    Patient* p; Medicine* m; Staff* s; Record* r; Bed* b;

    fp = fopen("patients.txt", "w");
    if (fp) {
        p = patientHead->next;
        while (p) {
            fprintf(fp, "%s %s %s %s %d %s %d %.2f\n", p->id, p->password, p->name, p->gender, p->age, p->allergy, p->isEmergency, p->balance);
            p = p->next;
        }
        fclose(fp);
    }

    fp = fopen("medicines.txt", "w");
    if (fp) {
        m = medicineHead->next;
        while (m) {
            fprintf(fp, "%s %s %d %.2f %s\n", m->id, m->name, m->stock, m->price, m->expiryDate);
            m = m->next;
        }
        fclose(fp);
    }

    fp = fopen("staff.txt", "w");
    if (fp) {
        s = staffHead->next;
        while (s) {
            fprintf(fp, "%s %s %s %s %s\n", s->id, s->password, s->name, s->department, s->level);
            s = s->next;
        }
        fclose(fp);
    }

    fp = fopen("records.txt", "w");
    if (fp) {
        r = recordHead->next;
        while (r) {
            fprintf(fp, "%s %d %s %s %.2f %d %s %s\n", r->recordId, r->type, r->patientId, r->staffId, r->cost, r->isPaid, r->description, r->createTime);
            r = r->next;
        }
        fclose(fp);
    }

    fp = fopen("beds.txt", "w");
    if (fp) {
        b = bedHead->next;
        while (b) {
            fprintf(fp, "%s %d %s %s %s %.2f %d\n", b->bedId, b->isOccupied, strlen(b->patientId) > 0 ? b->patientId : "none", b->wardType, b->bedType, b->price, b->isRoundsDone);
            b = b->next;
        }
        fclose(fp);
    }
}