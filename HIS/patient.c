#define _CRT_SECURE_NO_WARNINGS
#include "patient.h" // (注意里面需包含 void patientMenu(User* user); void patientRegister(); )
#include "utils.h"

// 内部工具：通过账号查找患者实名档案
Patient* getPatientInfo(const char* account) {
    Patient* p = patientHead;
    while (p) {
        if (strcmp(p->id, account) == 0) return p;
        p = p->next;
    }
    return NULL;
}

// 1. 患者建档
void patientRegister() {
    char name[50], pwd[50], new_account[20];
    printf("\n--- 患者注册与建档 ---\n1. 急诊 (极简录入)\n2. 普通 (详细录入)\n选择: ");
    int choice = safeGetInt();

    printf("请输入姓名: "); safeGetString(name, 50);
    printf("请设置登录密码: "); safeGetString(pwd, 50);
    sprintf(new_account, "P%d", rand() % 9000 + 1000);

    // 建立实名档案
    Patient* p = (Patient*)malloc(sizeof(Patient));
    strcpy(p->id, new_account);
    strcpy(p->name, name);
    p->balance = 0.0;
    p->isEmergency = (choice == 1) ? 1 : 0;

    if (choice == 2) {
        char ageStr[10];
        printf("性别: "); safeGetString(p->gender, 10);
        printf("年龄: "); safeGetString(ageStr, 10); p->age = atoi(ageStr);
        printf("过敏史: "); safeGetString(p->allergy, 100);
    }
    else {
        strcpy(p->gender, "未知");
        p->age = -1;
        strcpy(p->allergy, "未知");
    }
    p->next = patientHead; patientHead = p;

    // 建立登录账号
    User* u = (User*)malloc(sizeof(User));
    strcpy(u->account, new_account);
    strcpy(u->password, pwd);
    strcpy(u->role, "patient");
    strcpy(u->name, name);
    strcpy(u->department, "无");
    u->next = userHead; userHead = u;

    FILE* fp = fopen("user_data.txt", "a");
    if (fp) {
        fprintf(fp, "%s %s patient %s 无 0.0\n", new_account, pwd, name);
        fclose(fp);
    }

    printf("=> 建档成功！您的患者登录账号是: %s (请牢记用于登录)\n", new_account);
    system("pause");
}

// 2. 自助预约挂号
void patientBooking(User* user) {
    printf("\n--- 自助预约挂号 ---\n请输入科室或医生姓名进行模糊查询: ");
    char keyword[50]; safeGetString(keyword, 50);

    Schedule* s = scheduleHead;
    int found = 0;
    while (s) {
        if (strstr(s->name, keyword) || strstr(s->department, keyword)) {
            printf("工号:[%s] 姓名:%s 科室:%s 时间:%s %s 状态:%s\n",
                s->docId, s->name, s->department, s->date, s->timeSlot, s->status);
            found++;
        }
        s = s->next;
    }
    if (found == 0) { printf("未找到匹配排班。\n"); system("pause"); return; }

    printf("请输入要挂号的医生工号: ");
    char d_id[20]; safeGetString(d_id, 20);

    s = scheduleHead;
    while (s) {
        if (strcmp(s->docId, d_id) == 0 && strcmp(s->status, "满员") == 0) {
            printf("\n=> 该医生当前已满员！基于历史记录，推荐本科室其他专家：\n");
            Schedule* alt = scheduleHead;
            while (alt) {
                if (strcmp(alt->department, s->department) == 0 && strcmp(alt->docId, s->docId) != 0) {
                    printf("医生 [%s] %s\n", alt->docId, alt->name);
                    break;
                }
                alt = alt->next;
            }
            printf("是否接受推荐专家号？(1.是 0.否): ");
            if (safeGetInt() == 1 && alt) { strcpy(d_id, alt->docId); }
            else return;
        }
        s = s->next;
    }

    char desc[200]; sprintf(desc, "挂号预约:%s医师", d_id);
    // 生成 Type = 1 (挂号) 的记录
    saveRecordToFile("register_data.txt", 1, "挂号记录", user->account, d_id, desc, 15.0, 0);
    printf("=> 挂号成功！请前往财务中心缴费。\n");
    system("pause");
}

// 3. 财务与费用中心
void patientFinance(User* user) {
    Patient* p = getPatientInfo(user->account);
    if (!p) { printf("内部档案查询异常！\n"); return; }

    printf("\n--- 财务与费用中心 ---\n您的当前余额: %.2f 元\n", p->balance);

    double total_unpaid = 0;
    Record* r = recordHead;
    printf("\n待支付账单列表:\n");
    while (r) {
        if (strcmp(r->patientId, user->account) == 0 && r->isPaid == 0) {
            printf("- [%s] %s : %s (金额: %.2f元)\n", r->recordId, r->typeName, r->description, r->cost);
            total_unpaid += r->cost;
        }
        r = r->next;
    }

    if (total_unpaid == 0) { printf("=> 您没有未缴账单。\n"); system("pause"); return; }

    printf("总待缴金额: %.2f 元\n", total_unpaid);
    if (p->balance >= total_unpaid) {
        printf("1. 一键扣款缴费\n2. 暂不处理\n选择: ");
        if (safeGetInt() == 1) {
            p->balance -= total_unpaid;
            r = recordHead;
            while (r) {
                if (strcmp(r->patientId, user->account) == 0) r->isPaid = 1;
                r = r->next;
            }
            printf("=> 缴费成功！当前余额: %.2f\n", p->balance);
        }
    }
    else {
        printf("=> 余额不足，请充值。\n请输入充值金额 (>0): ");
        double amt = safeGetDouble();
        if (amt > 0) {
            p->balance += amt;
            printf("=> 充值成功！当前余额: %.2f，请重新进入缴费。\n", p->balance);
        }
        else {
            printf("=> 非法拦截，充值失败。\n");
        }
    }
    system("pause");
}

void patientMenu(User* user) {
    int choice;
    do {
        system("cls");
        printf("=== 患者端主界面 | 欢迎，%s ===\n", user->name);
        printf("1. 自助预约与挂号\n2. 财务与费用中心\n3. 个人医疗档案库\n0. 退出登录\n请选择: ");
        choice = safeGetInt();
        switch (choice) {
        case 1: patientBooking(user); break;
        case 2: patientFinance(user); break;
        case 3:
            printf("\n--- 医疗档案 (数据清洗与汇总) ---\n");
            Record* r = recordHead;
            while (r) {
                if (strcmp(r->patientId, user->account) == 0) {
                    printf("[%s] %s | %s | 金额:%.2f | 状态:%s\n",
                        r->recordId, r->typeName, r->description, r->cost, r->isPaid ? "已处理" : "待处理");
                }
                r = r->next;
            }
            system("pause"); break;
        }
    } while (choice != 0);
}