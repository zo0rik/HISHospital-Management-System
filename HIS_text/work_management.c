#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "models.h"
#include "utils.h"
#include "work_management.h"

void workManagementMenu(const char* docId) {
    while (1) {
        system("cls");
        printf("\n===== 工作管理 =====\n");

        typedef struct { char id[20]; char name[100]; } PatInfo;
        PatInfo pats[100]; int pCount = 0;

        Record* r = recordHead->next;
        while (r) {
            if (strcmp(r->staffId, docId) == 0) {
                int exists = 0;
                for (int i = 0; i < pCount; i++) { if (strcmp(pats[i].id, r->patientId) == 0) { exists = 1; break; } }
                if (!exists) {
                    strcpy(pats[pCount].id, r->patientId);
                    Patient* p = patientHead->next; strcpy(pats[pCount].name, "未知");
                    while (p) { if (strcmp(p->id, r->patientId) == 0) { strcpy(pats[pCount].name, p->name); break; } p = p->next; }
                    pCount++;
                }
            }
            r = r->next;
        }

        if (pCount == 0) { printf("您当前暂无任何接诊记录。\n"); system("pause"); return; }

        printf("--- 您曾接诊过的患者列表 ---\n");
        for (int i = 0; i < pCount; i++) printf("[%d] 患者ID: %s | 姓名: %s\n", i + 1, pats[i].id, pats[i].name);
        printf("[0] 返回上级菜单\n请输入患者对应编号进行操作: ");
        int pChoice = safeGetInt();
        if (pChoice == 0) return;
        if (pChoice < 1 || pChoice > pCount) continue;

        char* targetPid = pats[pChoice - 1].id;

        while (1) {
            system("cls");
            printf("\n--- 患者 %s (%s) 名下由您创建的诊疗记录 ---\n", pats[pChoice - 1].name, targetPid);

            typedef struct { char recId[30]; int type; char desc[300]; } RecInfo;
            RecInfo recs[100]; int rCount = 0;

            r = recordHead->next;
            while (r) {
                if (strcmp(r->staffId, docId) == 0 && strcmp(r->patientId, targetPid) == 0) {
                    strcpy(recs[rCount].recId, r->recordId);
                    recs[rCount].type = r->type;
                    strcpy(recs[rCount].desc, r->description);
                    rCount++;
                }
                r = r->next;
            }

            if (rCount == 0) { printf("该患者名下暂无您的记录。\n"); system("pause"); break; }

            for (int i = 0; i < rCount; i++) {
                char tName[30];
                switch (recs[i].type) {
                case 1: strcpy(tName, "① 挂号记录"); break;
                case 2: strcpy(tName, "② 看诊记录"); break;
                case 3: strcpy(tName, "③ 开药记录"); break;
                case 4: strcpy(tName, "④ 检查记录"); break;
                case 5: strcpy(tName, "⑤ 住院记录"); break;
                case 6: strcpy(tName, "⑥ 住院通知单"); break;
                }
                printf("[%d] %s | 流水号: %s | 详情: %s\n", i + 1, tName, recs[i].recId, recs[i].desc);
            }

            printf("\n操作: 1.修改记录 2.删除记录 0.返回重选患者\n请选择: ");
            int op = safeGetInt();
            if (op == 0) break;
            if (op == 1 || op == 2) {
                printf("请输入要操作的【记录编号】: ");
                int rChoice = safeGetInt();
                if (rChoice < 1 || rChoice > rCount) continue;

                char* targetRecId = recs[rChoice - 1].recId;
                Record* prev = recordHead; Record* curr = recordHead->next;
                while (curr) {
                    if (strcmp(curr->recordId, targetRecId) == 0) {
                        if (op == 1) {
                            printf("\n原详情: %s\n请输入新的描述信息(注意不包含空格): ", curr->description);
                            safeGetString(curr->description, 300);
                            printf("修改成功！(内存级实时同步)\n");
                        }
                        else {
                            prev->next = curr->next; free(curr);
                            printf("删除成功！(内存级实时同步)\n");
                        }
                        break;
                    }
                    prev = curr; curr = curr->next;
                }
                system("pause");
            }
        }
    }
}