#define _CRT_SECURE_NO_WARNINGS
#include "drug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 队友自己定义的管理端独立药房链表与历史记录链表
Drug* drugList = NULL;
DrugHistory* drugHistoryList = NULL;

// ---------------------------------------------------------
// 从本地加载管理端的药品数据
// ---------------------------------------------------------
void loadDrugs() {
    FILE* fp = fopen("drugs.txt", "r");
    if (!fp) return;  // 文件不存在，则从空链表开始

    char line[512];
    Drug d;
    Drug* tail = NULL;
    // 按行读取并使用 strtok 按照逗号分割解析字段
    while (fgets(line, sizeof(line), fp)) {
        // 去掉行尾换行符
        line[strcspn(line, "\n")] = 0;

        char* token = strtok(line, ",");
        if (token) strcpy(d.id,token); else d.id[0] = '\0';
        token = strtok(NULL, ",");
        if (token) strcpy(d.name, token); else d.name[0] = '\0';//����
        token = strtok(NULL, ",");
		if (token) d.stock = atoi(token); else d.stock = 0;//���
        token = strtok(NULL, ",");
		if (token) d.price = atof(token); else d.price = 0.0;//�۸�
        token = strtok(NULL, ",");
		if (token) strcpy(d.batch, token); else d.batch[0] = '\0';//����
        token = strtok(NULL, ",");
		if (token) strcpy(d.expiry, token); else d.expiry[0] = '\0';//��Ч��
        token = strtok(NULL, ",");
		if (token) strcpy(d.last_in, token); else d.last_in[0] = '\0';//������ʱ��
        token = strtok(NULL, ",");
		if (token) strcpy(d.last_out, token); else d.last_out[0] = '\0';//�������ʱ��

        // 尾插法构建药品链表
        Drug* node = (Drug*)malloc(sizeof(Drug));
        *node = d;
        node->next = NULL;
        if (!drugList) drugList = tail = node;
        else { tail->next = node; tail = node; }
    }
    fclose(fp);
}

// ---------------------------------------------------------
// 保存管理端的药品数据到本地
// ---------------------------------------------------------
void saveDrugs() {
    FILE* fp = fopen("drugs.txt", "w");
    if (!fp) return;
    Drug* p = drugList;
    while (p) {
        fprintf(fp, "%s,%s,%d,%.2f,%s,%s,%s,%s\n",
            p->id, p->name, p->stock, p->price,
            p->batch, p->expiry, p->last_in, p->last_out);
        p = p->next;
    }
    fclose(fp);
}

// --------------------------------------------------------
// ����ҩƷ�����䶯��ʷ��¼
// --------------------------------------------------------
void loadDrugHistory() {
    FILE* fp = fopen("drug_history.txt", "r");
    if (!fp) return;

    char line[512];
    DrugHistory h;
    DrugHistory* tail = NULL;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        char* token = strtok(line, ",");
        if (token) strcpy(h.drug_id,token); else h.drug_id[0] = '\0';
        token = strtok(NULL, ",");
        if (token) h.type = atoi(token); else h.type = 0;
        token = strtok(NULL, ",");
        if (token) h.quantity = atoi(token); else h.quantity = 0;
        token = strtok(NULL, ",");
        if (token) strcpy(h.time, token); else h.time[0] = '\0';

        DrugHistory* node = (DrugHistory*)malloc(sizeof(DrugHistory));
        *node = h;
        node->next = NULL;
        if (!drugHistoryList) drugHistoryList = tail = node;
        else { tail->next = node; tail = node; }
    }
    fclose(fp);
}

// ---------------------------------------------------------
// 保存药品出入库变动历史记录
// ---------------------------------------------------------
void saveDrugHistory() {
    FILE* fp = fopen("drug_history.txt", "w");
    if (!fp) return;
    DrugHistory* p = drugHistoryList;
    while (p) {
        fprintf(fp, "%s,%d,%d,%s\n", p->drug_id, p->type, p->quantity, p->time);
        p = p->next;
    }
    fclose(fp);
}

// ---------------------------------------------------------
// 内部工具：格式化打印所有药品信息
// ---------------------------------------------------------
static void displayAllDrugs() {
    if (!drugList) {
        printf("药品库为空。\n");
        return;
    }
    printf("\n--- ҩƷ�б� ---\n");
    printf("%-15s %-20s %-10s %-8s %-12s %-12s %-20s %-20s\n", "ID", "����", "���", "�۸�", "����", "��Ч��", "������", "�������");
    Drug* p = drugList;
    while (p) {
        printf("%-15s %-20s %-10d %-8.2f %-12s %-12s %-20s %-20s\n",
            p->id, p->name, p->stock, p->price, p->batch, p->expiry,
            p->last_in, p->last_out);
        p = p->next;
    }
}

// ---------------------------------------------------------
// 业务一：药品库存多维度查询
// ---------------------------------------------------------
static void drugStockQuery() {
    int choice;
    printf("\n药品库存查询:\n1-按ID查询\n2-按名称模糊查询\n3-查看所有药品\n请选择: ");
    scanf("%d", &choice);

    // 按精确ID检索
    if (choice == 1) {
        char id[15];
        for(int i=0;i<15;i++)   
			id[i]= '\0'; // ȷ���ַ�����β]
        printf("������ҩƷID: "); scanf("%s", id);
        if (id[14] != '\0') {
            printf("δ�ҵ���ҩƷ��\n");
            return ;
        }
        Drug* p = drugList;
        while (p) {
            if (strcmp(p->id,id)==0){
                printf("ID:%s ����:%s ���:%d �۸�:%.2f ����:%s ��Ч��:%s\n",
                    p->id, p->name, p->stock, p->price, p->batch, p->expiry);
                return;
            }
            p = p->next;
        }
        printf("未找到该药品。\n");
    }
    // 按名称关键字模糊检索
    else if (choice == 2) {
        char name[50]; printf("请输入药品名称关键字: "); scanf("%s", name);
        int found = 0;
        Drug* p = drugList;
        while (p) {
            if (strstr(p->name, name)) {
                printf("ID:%d 名称:%s 库存:%d 价格:%.2f 批号:%s 有效期:%s\n",
                    p->id, p->name, p->stock, p->price, p->batch, p->expiry);
                found = 1;
            }
            p = p->next;
        }
        if (!found) printf("未找到匹配药品。\n");
    }
    else if (choice == 3) {
        displayAllDrugs();
    }
    else printf("无效选择。\n");
}

// ---------------------------------------------------------
// 业务二：查看历史库存流水记录
// ---------------------------------------------------------
static void viewStockRecords() {
    printf("\n--- 库存变动记录 ---\n");
    DrugHistory* h = drugHistoryList;
    if (!h) { printf("���޼�¼��\n"); return; }
    printf("%-15s %-6s %-8s %-20s\n", "ҩƷID", "����", "����", "ʱ��");
    while (h) {
        printf("%-15s %-6s %-8d %-20s\n", h->drug_id,
            (h->type == 1) ? "���" : "����", h->quantity, h->time);
        h = h->next;
    }
}

// ---------------------------------------------------------
// 业务三：药品入库管理 (增加库存并记录流水)
// ---------------------------------------------------------
static void drugIn() {
    int  quantity;
    char id[15];
    printf("������ҩƷID: ");
    id[14] = '\0'; // ȷ���ַ�����β
    scanf("%s", id);
    if (id[14]!='\0'){
		printf("δ�ҵ���ҩƷ��\n");
        return;
    }
    Drug* p = drugList;
    while (p) {
        if (strcmp(p->id,id)==0) {
            printf("��ǰ���: %d\n", p->stock);
            printf("�������������: ");
            scanf("%d", &quantity);
            if (quantity <= 0) { printf("数量必须为正。\n"); return; }

            p->stock += quantity; // 增加物理库存
            getCurrentTime(p->last_in, 30); // 更新最近入库时间

            // 采用头插法将变动记录插入历史流水链表
            DrugHistory* h = (DrugHistory*)malloc(sizeof(DrugHistory));
            strcpy(h->drug_id,id);
            h->type = 1;
            h->quantity = quantity;
            getCurrentTime(h->time, 30);
            h->next = drugHistoryList;
            drugHistoryList = h;
            printf("入库成功，新库存: %d\n", p->stock);
            return;
        }
        p = p->next;
    }
    printf("未找到该药品。\n");
}

// ---------------------------------------------------------
// 业务四：药品人工出库管理
// ---------------------------------------------------------
static void drugOut() {
    int  quantity;
    char id[15];
    printf("������ҩƷID: ");
    id[14] = '\0'; // ȷ���ַ�����β
    scanf("%s", id);
    if (id[14] != '\0') {
        printf("δ�ҵ���ҩƷ��\n");
        return;
	}//��ֹ������������ڴ�Խ��
    Drug* p = drugList;
    while (p) {
        if (strcmp(p->id,id)==0) {
            printf("��ǰ���: %d\n", p->stock);
            printf("�������������: ");
            scanf("%d", &quantity);
            if (quantity <= 0) { printf("数量必须为正。\n"); return; }
            if (p->stock < quantity) { printf("库存不足！\n"); return; }

            p->stock -= quantity; // 扣减物理库存
            getCurrentTime(p->last_out, 30);

            // 生成流水记录
            DrugHistory* h = (DrugHistory*)malloc(sizeof(DrugHistory));
            strcpy(h->drug_id ,id);
            h->type = 2; // 2��������
            h->quantity = quantity;
            getCurrentTime(h->time, 30);
            h->next = drugHistoryList;
            drugHistoryList = h;
            printf("出库成功，新库存: %d\n", p->stock);
            return;
        }
        p = p->next;
    }
    printf("未找到该药品。\n");
}
// ---------------------------------------------------------
// ҵ���壺������ҩƷ��Ϣ
//----------------------------------------------------------
void addDrug() {
    Drug d;
    printf("��������ҩƷID: ");
    d.id[14] = '\0'; // ȷ���ַ�����β
    scanf("%s", d.id);
    if (d.id[14] != '\0') {
        printf("ҩƷID����������ʧ�ܡ�\n");
        return;
    }
    printf("������ҩƷ����: ");
    scanf("%s", d.name);
    printf("�������ʼ�������: ");
    scanf("%d", &d.stock);
    printf("������ҩƷ�۸�: ");
    scanf("%f", &d.price);
    printf("����������: ");
    scanf("%s", d.batch);
    printf("��������Ч�� (��2025-12-31): ");
    scanf("%s", d.expiry);
    getCurrentTime(d.last_in, 30); // ��ʼ���ʱ��Ϊ��
    strcpy(d.last_out, ""); // ��ʼ����ʱ��Ϊ��
    // ����ҩƷ���ӵ�����ĩβ
    Drug* node = (Drug*)malloc(sizeof(Drug));
    *node = d;
    node->next = NULL;
    if (!drugList) drugList = node;
    else {
        Drug* p = drugList;
        while (p->next) p = p->next;
        p->next = node;
    }
    printf("��ҩƷ���ӳɹ���\n");
}
// ---------------------------------------------------------
// ҵ���壺������ҩƷ��Ϣ
//----------------------------------------------------------
void addDrug() {
    Drug d;
    printf("��������ҩƷID: ");
    d.id[14] = '\0'; // ȷ���ַ�����β
    scanf("%s", d.id);
    if (d.id[14] != '\0') {
        printf("ҩƷID����������ʧ�ܡ�\n");
        return;
    }
    printf("������ҩƷ����: ");
    scanf("%s", d.name);
    printf("�������ʼ�������: ");
    scanf("%d", &d.stock);
    printf("������ҩƷ�۸�: ");
    scanf("%f", &d.price);
    printf("����������: ");
    scanf("%s", d.batch);
    printf("��������Ч�� (��2025-12-31): ");
    scanf("%s", d.expiry);
    getCurrentTime(d.last_in, 30); // ��ʼ���ʱ��Ϊ��
    strcpy(d.last_out, ""); // ��ʼ����ʱ��Ϊ��
    // ����ҩƷ���ӵ�����ĩβ
    Drug* node = (Drug*)malloc(sizeof(Drug));
    *node = d;
    node->next = NULL;
    if (!drugList) drugList = node;
    else {
        Drug* p = drugList;
        while (p->next) p = p->next;
        p->next = node;
    }
    printf("��ҩƷ���ӳɹ���\n");
}

// ---------------------------------------------------------
// 管理端：药房系统子路由
// ---------------------------------------------------------
void drugMenu() {
    int choice;
    do {
        // ע�⣺�����¼���أ�ֱ�ӳ���ҵ��˵�
        printf("\n========== ҩ������ ==========\n");
        printf("1. ҩƷ����ѯ\n");
        printf("2. ҩƷ���\n");
        printf("3. ҩƷ����\n");
        printf("4. �鿴���䶯��¼\n");
		printf("5.����ҩƷ��Ϣ\n");
        printf("0. �������˵�\n");
        printf("��ѡ��: ");
        scanf("%d", &choice);
        switch (choice) {
        case 1: drugStockQuery(); break;
        case 2: drugIn(); break;
        case 3: drugOut(); break;
        case 4: viewStockRecords(); break;
		case 5: addDrug(); break;
        case 0: break;
        default: printf("无效选项。\n");
        }
    } while (choice != 0);
}