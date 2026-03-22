#define _CRT_SECURE_NO_WARNINGS
#include "models.h"
#include "utils.h"

void safeGetString(char* buffer, int size) {
    if (fgets(buffer, size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
    }
    else {
        buffer[0] = '\0';
    }
}

int safeGetInt() {
    char buffer[100];
    safeGetString(buffer, sizeof(buffer));
    int value = 0;
    sscanf(buffer, "%d", &value);
    return value;
}

double safeGetDouble() {
    char buffer[100];
    safeGetString(buffer, sizeof(buffer));
    double value = 0.0;
    sscanf(buffer, "%lf", &value);
    return value;
}

// 内部工具：添加系统登录账号节点
void addUserNode(const char* account, const char* pwd, const char* role, const char* name, const char* dept) {
    User* node = (User*)malloc(sizeof(User));
    strcpy(node->account, account);
    strcpy(node->password, pwd);
    strcpy(node->role, role);
    strcpy(node->name, name);
    strcpy(node->department, dept);
    node->next = NULL;
    if (!userHead) { userHead = node; return; }
    User* temp = userHead;
    while (temp->next) temp = temp->next;
    temp->next = node;
}

// 内部工具：添加患者详细档案节点
void addPatientNode(const char* id, const char* name, double balance) {
    Patient* p = (Patient*)malloc(sizeof(Patient));
    strcpy(p->id, id);
    strcpy(p->name, name);
    p->balance = balance;
    strcpy(p->gender, "未知");
    p->age = 0;
    strcpy(p->allergy, "无");
    p->isEmergency = 0;
    p->next = NULL;
    if (!patientHead) { patientHead = p; return; }
    Patient* temp = patientHead;
    while (temp->next) temp = temp->next;
    temp->next = p;
}

// 启动时自动加载基础数据到新版融合链表
void loadAllData() {
    FILE* fp; char s1[50], s2[50], s3[50], s4[50], s5[50], s6[50]; double f1; int i1, i2;

    // 1. 加载登录与患者信息 (txt里第6个字段顺便给患者充当余额)
    fp = fopen("user_data.txt", "r");
    if (fp) {
        while (fscanf(fp, "%s %s %s %s %s %lf", s1, s2, s3, s4, s5, &f1) != EOF) {
            addUserNode(s1, s2, s3, s4, s5);
            if (strcmp(s3, "patient") == 0) {
                addPatientNode(s1, s4, f1); // 同步创建患者档案
            }
        }
        fclose(fp);
    }
    else printf("[系统提示] user_data.txt 不存在，将启动空环境。\n");

    // 2. 加载药品信息 (适配新字段：通用名 commonName 和 关联科室 department)
    fp = fopen("medicine_data.txt", "r");
    if (fp) {
        while (fscanf(fp, "%s %s %s %d %lf %s", s1, s2, s3, &i1, &f1, s4) != EOF) {
            Medicine* m = (Medicine*)malloc(sizeof(Medicine));
            strcpy(m->id, s1); strcpy(m->name, s2); strcpy(m->commonName, s3);
            m->stock = i1; m->price = f1; strcpy(m->department, s4); strcpy(m->expiryDate, "2026-12-31");
            m->next = medicineHead; medicineHead = m;
        }
        fclose(fp);
    }

    // 3. 加载排班信息 (适配新表 Schedule)
    fp = fopen("schedule_data.txt", "r");
    if (fp) {
        while (fscanf(fp, "%s %s %s %s %s %s", s1, s2, s3, s4, s5, s6) != EOF) {
            Schedule* s = (Schedule*)malloc(sizeof(Schedule));
            strcpy(s->docId, s1); strcpy(s->name, s2); strcpy(s->department, s3);
            strcpy(s->date, s4); strcpy(s->timeSlot, s5); strcpy(s->status, s6);
            s->next = scheduleHead; scheduleHead = s;
        }
        fclose(fp);
    }

    // 4. 加载病房信息 (适配新字段：病房类型 type 和 床位单价 price)
    fp = fopen("bed_data.txt", "r");
    if (fp) {
        while (fscanf(fp, "%s %s %s %d %d %lf %s", s1, s2, s3, &i1, &i2, &f1, s4) != EOF) {
            Bed* b = (Bed*)malloc(sizeof(Bed));
            strcpy(b->bedId, s1); strcpy(b->department, s2); strcpy(b->type, s3);
            b->bedNumber = i1; b->isOccupied = i2; b->price = f1; strcpy(b->patientId, s4);
            b->next = bedHead; bedHead = b;
        }
        fclose(fp);
    }
}

// 通用流水账单保存并同步至文件
void saveRecordToFile(const char* filename, int type, const char* typeName, const char* patientId, const char* staffId, const char* desc, double cost, int isPaid) {
    Record* r = (Record*)malloc(sizeof(Record));
    sprintf(r->recordId, "REC%d", rand() % 10000 + 1000);
    r->type = type;
    strcpy(r->typeName, typeName);
    strcpy(r->patientId, patientId);
    strcpy(r->staffId, staffId);
    strcpy(r->description, desc);
    r->cost = cost;
    r->isPaid = isPaid;

    // 头插法记入内存
    r->next = recordHead; recordHead = r;

    // 写入文件持久化
    FILE* fp = fopen(filename, "a");
    if (fp) {
        fprintf(fp, "%s %d %s %s %s %.2f %d %s\n", r->recordId, type, typeName, patientId, staffId, cost, isPaid, desc);
        fclose(fp);
    }
}