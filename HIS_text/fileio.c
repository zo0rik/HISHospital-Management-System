#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fileio.h"
#include "models.h"
#include "utils.h"
#include "drug.h"
#include "doctor.h"
#include "admin.h"
#include "schedule.h"
#include "transaction.h"

static void copyField(char* dest, size_t destSize, const char* src) {
    if (!dest || destSize == 0) return;
    if (!src) { dest[0] = '\0'; return; }
    strncpy(dest, src, destSize - 1);
    dest[destSize - 1] = '\0';
}


void loadAllDataFromTxt() {
    FILE* fp;

    fp = fopen("patients.txt", "r");
    if (fp) {
        char line[1024];
        Patient* p_tail = patientHead;
        while (p_tail->next != NULL) p_tail = p_tail->next;

        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = 0;
            if (strlen(line) == 0) continue;

            Patient p_temp = { 0 };
            char* token = strtok(line, ",");
            if (token) copyField(p_temp.id, sizeof(p_temp.id), token);
            token = strtok(NULL, ","); if (token) copyField(p_temp.password, sizeof(p_temp.password), token);
            token = strtok(NULL, ","); if (token) copyField(p_temp.name, sizeof(p_temp.name), token);
            token = strtok(NULL, ","); if (token) copyField(p_temp.gender, sizeof(p_temp.gender), token);
            token = strtok(NULL, ","); if (token) p_temp.age = atoi(token);
            token = strtok(NULL, ","); if (token) copyField(p_temp.allergy, sizeof(p_temp.allergy), token);
            token = strtok(NULL, ","); if (token) p_temp.isEmergency = atoi(token);
            token = strtok(NULL, ","); if (token) p_temp.balance = atof(token); else p_temp.balance = 0.0;
            token = strtok(NULL, ","); if (token) p_temp.inpatientDeposit = atof(token); else p_temp.inpatientDeposit = 0.0;
            token = strtok(NULL, ","); if (token) p_temp.isInpatient = atoi(token); else p_temp.isInpatient = 0;

            Patient* p_node = (Patient*)malloc(sizeof(Patient));
            *p_node = p_temp; p_node->next = NULL;
            p_tail->next = p_node; p_tail = p_node;
        }
        fclose(fp);
    }

    fp = fopen("doctor_information.txt", "r");
    if (fp) {
        char line[512];
        Staff* s_tail = staffHead;
        while (s_tail->next != NULL) s_tail = s_tail->next;

        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = 0;
            if (strlen(line) == 0) continue;

            Staff s_temp = { 0 };
            char* token = strtok(line, ",");
            if (token) copyField(s_temp.id, sizeof(s_temp.id), token);
            token = strtok(NULL, ","); if (token) copyField(s_temp.password, sizeof(s_temp.password), token);
            token = strtok(NULL, ","); if (token) copyField(s_temp.name, sizeof(s_temp.name), token);
            token = strtok(NULL, ","); if (token) copyField(s_temp.department, sizeof(s_temp.department), token);
            token = strtok(NULL, ","); if (token) copyField(s_temp.level, sizeof(s_temp.level), token);
            token = strtok(NULL, ","); if (token) copyField(s_temp.sex, sizeof(s_temp.sex), token);

            Staff* s_node = (Staff*)malloc(sizeof(Staff));
            *s_node = s_temp; s_node->next = NULL;
            s_tail->next = s_node; s_tail = s_node;
        }
        fclose(fp);
    }

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
            if (token) copyField(r_temp.recordId, sizeof(r_temp.recordId), token);
            token = strtok(NULL, ","); if (token) r_temp.type = atoi(token);
            token = strtok(NULL, ","); if (token) copyField(r_temp.patientId, sizeof(r_temp.patientId), token);
            token = strtok(NULL, ","); if (token) copyField(r_temp.staffId, sizeof(r_temp.staffId), token);
            token = strtok(NULL, ","); if (token) r_temp.cost = atof(token);
            token = strtok(NULL, ","); if (token) r_temp.isPaid = atoi(token);
            token = strtok(NULL, ","); if (token) copyField(r_temp.description, sizeof(r_temp.description), token);
            token = strtok(NULL, ","); if (token) copyField(r_temp.createTime, sizeof(r_temp.createTime), token);

            Record* r_node = (Record*)malloc(sizeof(Record));
            *r_node = r_temp; r_node->next = NULL;
            r_tail->next = r_node; r_tail = r_node;
        }
        fclose(fp);
    }

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
            if (token) copyField(b_temp.bedId, sizeof(b_temp.bedId), token);
            token = strtok(NULL, ","); if (token) b_temp.isOccupied = atoi(token);
            token = strtok(NULL, ",");
            if (token) {
                /* 【BUG修复7】保存时空patientId写为"无"，加载时需还原为空串 */
                if (strcmp(token, "无") == 0)
                    strcpy(b_temp.patientId, "");
                else
                    copyField(b_temp.patientId, sizeof(b_temp.patientId), token);
            }
            token = strtok(NULL, ","); if (token) copyField(b_temp.wardType, sizeof(b_temp.wardType), token);
            token = strtok(NULL, ","); if (token) copyField(b_temp.bedType, sizeof(b_temp.bedType), token);
            token = strtok(NULL, ","); if (token) b_temp.price = atof(token);
            token = strtok(NULL, ","); if (token) b_temp.isRoundsDone = atoi(token);

            Bed* b_node = (Bed*)malloc(sizeof(Bed));
            *b_node = b_temp; b_node->next = NULL;
            b_tail->next = b_node; b_tail = b_node;
        }
        fclose(fp);
    }
}

void saveAllDataToTxt() {
    FILE* fp;

    fp = fopen("patients.txt", "w");
    if (fp) {
        Patient* p = patientHead->next;
        while (p) {
            fprintf(fp, "%s,%s,%s,%s,%d,%s,%d,%.2f,%.2f,%d\n",
                p->id, p->password, p->name, p->gender,
                p->age, p->allergy, p->isEmergency, p->balance,
                p->inpatientDeposit, p->isInpatient);
            p = p->next;
        }
        fclose(fp);
    }

    fp = fopen("doctor_information.txt", "w");
    if (fp) {
        Staff* s = staffHead->next;
        while (s) {
            fprintf(fp, "%s,%s,%s,%s,%s,%s\n",
                s->id, s->password, s->name, s->department, s->level, s->sex);
            s = s->next;
        }
        fclose(fp);
    }

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

    fp = fopen("beds.txt", "w");
    if (fp) {
        Bed* b = bedHead->next;
        while (b) {
            char safePatientId[30];
            if (strlen(b->patientId) == 0) strcpy(safePatientId, "无");
            else strcpy(safePatientId, b->patientId);

            fprintf(fp, "%s,%d,%s,%s,%s,%.2f,%d\n",
                b->bedId, b->isOccupied, safePatientId,
                b->wardType, b->bedType, b->price, b->isRoundsDone);
            b = b->next;
        }
        fclose(fp);
    }
}


// =========================================================================
// 2. 药品与库存读写 (转移自 drug.c)
// =========================================================================
void loadDrugs() {
    FILE* fp = fopen("drug.txt", "r");
    if (!fp) return;

    char line[512];
    Drug d;
    Drug* tail = drugList;
    while (tail->next != NULL) tail = tail->next;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        char* token = strtok(line, ",");
        if (token) d.id = atoi(token); else d.id = 0;
        token = strtok(NULL, ","); if (token) copyField(d.name, sizeof(d.name), token); else d.name[0] = '\0';
        token = strtok(NULL, ","); if (token) d.stock = atoi(token); else d.stock = 0;
        token = strtok(NULL, ","); if (token) d.price = atof(token); else d.price = 0.0;
        token = strtok(NULL, ","); if (token) copyField(d.batch, sizeof(d.batch), token); else d.batch[0] = '\0';
        token = strtok(NULL, ","); if (token) copyField(d.expiry, sizeof(d.expiry), token); else d.expiry[0] = '\0';
        token = strtok(NULL, ","); if (token) copyField(d.last_in, sizeof(d.last_in), token); else d.last_in[0] = '\0';
        token = strtok(NULL, ","); if (token) copyField(d.last_out, sizeof(d.last_out), token); else d.last_out[0] = '\0';

        Drug* node = (Drug*)malloc(sizeof(Drug));
        if (!node) break;
        *node = d;
        node->next = NULL;
        tail->next = node;
        tail = node;
    }
    fclose(fp);
}

void saveDrugs() {
    FILE* fp = fopen("drug.txt", "w");
    if (!fp) return;
    Drug* p = drugList->next;
    while (p) {
        fprintf(fp, "%d,%s,%d,%.2f,%s,%s,%s,%s\n",
            p->id, p->name, p->stock, p->price,
            p->batch, p->expiry, p->last_in, p->last_out);
        p = p->next;
    }
    fclose(fp);
}

void loadDrugHistory() {
    FILE* fp = fopen("drug_history.txt", "r");
    if (!fp) return;

    char line[512];
    DrugHistory h;
    DrugHistory* tail = drugHistoryList;
    while (tail->next != NULL) tail = tail->next;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        char* token = strtok(line, ",");
        if (token) h.drug_id = atoi(token); else h.drug_id = 0;
        token = strtok(NULL, ","); if (token) h.type = atoi(token); else h.type = 0;
        token = strtok(NULL, ","); if (token) h.quantity = atoi(token); else h.quantity = 0;
        token = strtok(NULL, ","); if (token) copyField(h.time, sizeof(h.time), token); else h.time[0] = '\0';

        DrugHistory* node = (DrugHistory*)malloc(sizeof(DrugHistory));
        if (!node) break;
        *node = h;
        node->next = NULL;
        tail->next = node;
        tail = node;
    }
    fclose(fp);
}

void saveDrugHistory() {
    FILE* fp = fopen("drug_history.txt", "w");
    if (!fp) return;
    DrugHistory* p = drugHistoryList->next;
    while (p) {
        fprintf(fp, "%d,%d,%d,%s\n", p->drug_id, p->type, p->quantity, p->time);
        p = p->next;
    }
    fclose(fp);
}


// =========================================================================
// 4. 管理员数据读写 
// =========================================================================
void loadAdminData(void) {
    FILE* fp;
    char line[256];
    Admin* tail;

    freeAdminList();
    adminHead = (Admin*)calloc(1, sizeof(Admin));
    if (!adminHead) return;
    adminHead->next = NULL;
    tail = adminHead;

    fp = fopen("admin.txt", "r");
    if (!fp) {
        Admin* node = (Admin*)calloc(1, sizeof(Admin));
        if (!node) return;
        copyField(node->username, sizeof(node->username), "admin");
        copyField(node->password, sizeof(node->password), "admin123");
        copyField(node->phone, sizeof(node->phone), "13800000000");
        copyField(node->email, sizeof(node->email), "admin@hospital.com");
        node->next = NULL;
        adminHead->next = node;
        setCurrentAdminNode(node);
        return;
    }

    while (fgets(line, sizeof(line), fp)) {
        Admin* node;
        char* token;

        line[strcspn(line, "\n")] = 0;
        if (line[0] == '\0') continue;

        node = (Admin*)calloc(1, sizeof(Admin));
        if (!node) break;

        token = strtok(line, ",");
        if (token) copyField(node->username, sizeof(node->username), token);
        token = strtok(NULL, ",");
        if (token) copyField(node->password, sizeof(node->password), token);
        token = strtok(NULL, ",");
        if (token) copyField(node->phone, sizeof(node->phone), token);
        token = strtok(NULL, ",");
        if (token) copyField(node->email, sizeof(node->email), token);

        if (node->username[0] == '\0') {
            free(node);
            continue;
        }
        node->next = NULL;
        tail->next = node;
        tail = node;
    }
    fclose(fp);

    if (!adminHead->next) {
        Admin* node = (Admin*)calloc(1, sizeof(Admin));
        if (!node) return;
        copyField(node->username, sizeof(node->username), "admin");
        copyField(node->password, sizeof(node->password), "admin123");
        copyField(node->phone, sizeof(node->phone), "13800000000");
        copyField(node->email, sizeof(node->email), "admin@hospital.com");
        node->next = NULL;
        adminHead->next = node;
    }
    setCurrentAdminNode(adminHead->next);
}

void saveAdminData(void) {
    FILE* fp = fopen("admin.txt", "w");
    Admin* p;
    if (!fp) return;

    if (currentAdminNode) {
        copyField(currentAdminNode->username, sizeof(currentAdminNode->username), admin.username);
        copyField(currentAdminNode->password, sizeof(currentAdminNode->password), admin.password);
        copyField(currentAdminNode->phone, sizeof(currentAdminNode->phone), admin.phone);
        copyField(currentAdminNode->email, sizeof(currentAdminNode->email), admin.email);
    }

    p = adminHead ? adminHead->next : NULL;
    while (p) {
        fprintf(fp, "%s,%s,%s,%s\n", p->username, p->password, p->phone, p->email);
        p = p->next;
    }
    fclose(fp);
}

// =========================================================================
// 5. 排班与流水读写 
// =========================================================================

void loadSchedules() {
    FILE* fp = fopen("schedules.txt", "r");
    if (!fp) return;

    char line[256];
    Schedule s;
    Schedule* tail = scheduleList;
    while (tail->next != NULL) tail = tail->next;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        char* token = strtok(line, ",");
        if (token) s.schedule_id = atoi(token); else s.schedule_id = 0;
        token = strtok(NULL, ",");
        if (token) strncpy(s.doctor_id, token, sizeof(s.doctor_id) - 1); else s.doctor_id[0] = '\0';
        token = strtok(NULL, ",");
        if (token) copyField(s.date, sizeof(s.date), token); else s.date[0] = '\0';
        token = strtok(NULL, ",");
        if (token) copyField(s.shift, sizeof(s.shift), token); else s.shift[0] = '\0';

        Schedule* node = (Schedule*)malloc(sizeof(Schedule));
        *node = s;
        node->next = NULL;
        tail->next = node;
        tail = node;
    }
    fclose(fp);
}

void saveSchedules() {
    FILE* fp = fopen("schedules.txt", "w");
    if (!fp) return;
    Schedule* p = scheduleList->next;
    while (p) {
        fprintf(fp, "%d,%s,%s,%s\n", p->schedule_id, p->doctor_id, p->date, p->shift);
        p = p->next;
    }
    fclose(fp);
}

void loadTransactions() {
    FILE* fp = fopen("transactions.txt", "r");
    if (!fp) return;

    char line[512];
    Transaction t;
    Transaction* tail = transactionList;
    while (tail->next != NULL) tail = tail->next;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        char* token = strtok(line, ",");
        if (token) t.id = atoi(token); else t.id = 0;
        token = strtok(NULL, ",");
        if (token) t.type = atoi(token); else t.type = 0;
        token = strtok(NULL, ",");
        if (token) t.amount = atof(token); else t.amount = 0.0;
        token = strtok(NULL, ",");
        if (token) copyField(t.time, sizeof(t.time), token); else t.time[0] = '\0';
        token = strtok(NULL, ",");
        if (token) copyField(t.description, sizeof(t.description), token); else t.description[0] = '\0';

        Transaction* node = (Transaction*)malloc(sizeof(Transaction));
        *node = t;
        node->next = NULL;
        tail->next = node;
        tail = node;
    }
    fclose(fp);
}

void saveTransactions() {
    FILE* fp = fopen("transactions.txt", "w");
    if (!fp) return;
    Transaction* p = transactionList->next;
    while (p) {
        fprintf(fp, "%d,%d,%.2f,%s,%s\n", p->id, p->type, p->amount, p->time, p->description);
        p = p->next;
    }
    fclose(fp);
}