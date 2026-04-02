#define _CRT_SECURE_NO_WARNINGS
#include "drug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

<<<<<<< HEAD
// ¶ÓÓÑ×Ô¼º¶¨ÒåµÄ¹ÜÀí¶Ë¶ÀÁ¢Ò©·¿Á´±íÓëÀúÊ·¼ÇÂ¼Á´±í
=======
// ï¿½ï¿½ï¿½ï¿½ï¿½Ô¼ï¿½ï¿½ï¿½ï¿½ï¿½Ä¹ï¿½ï¿½ï¿½ï¿½Ë¶ï¿½ï¿½ï¿½Ò©ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ê·ï¿½ï¿½Â¼ï¿½ï¿½ï¿½ï¿?
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
Drug* drugList = NULL;
DrugHistory* drugHistoryList = NULL;

// ---------------------------------------------------------
<<<<<<< HEAD
// ´Ó±¾µØ¼ÓÔØ¹ÜÀí¶ËµÄÒ©Æ·Êý¾Ý
// ---------------------------------------------------------
void loadDrugs() {
    FILE* fp = fopen("drugs.txt", "r");
    if (!fp) return;  // ÎÄ¼þ²»´æÔÚ£¬Ôò´Ó¿ÕÁ´±í¿ªÊ¼
=======
// ï¿½Ó±ï¿½ï¿½Ø¼ï¿½ï¿½Ø¹ï¿½ï¿½ï¿½ï¿½Ëµï¿½Ò©Æ·ï¿½ï¿½ï¿½ï¿½
// ---------------------------------------------------------
void loadDrugs() {
    FILE* fp = fopen("drugs.txt", "r");
    if (!fp) return;  // ï¿½Ä¼ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ú£ï¿½ï¿½ï¿½Ó¿ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ê?
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2

    char line[512];
    Drug d;
    Drug* tail = NULL;
<<<<<<< HEAD
    // °´ÐÐ¶ÁÈ¡²¢Ê¹ÓÃ strtok °´ÕÕ¶ººÅ·Ö¸î½âÎö×Ö¶Î
    while (fgets(line, sizeof(line), fp)) {
        // È¥µôÐÐÎ²»»ÐÐ·û
=======
    // ï¿½ï¿½ï¿½Ð¶ï¿½È¡ï¿½ï¿½Ê¹ï¿½ï¿½ strtok ï¿½ï¿½ï¿½Õ¶ï¿½ï¿½Å·Ö¸ï¿½ï¿½ï¿½ï¿½ï¿½Ö¶ï¿?
    while (fgets(line, sizeof(line), fp)) {
        // È¥ï¿½ï¿½ï¿½ï¿½Î²ï¿½ï¿½ï¿½Ð·ï¿½
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
        line[strcspn(line, "\n")] = 0;

        char* token = strtok(line, ",");
        if (token) strcpy(d.id,token); else d.id[0] = '\0';
        token = strtok(NULL, ",");
        if (token) strcpy(d.name, token); else d.name[0] = '\0';//????????
        token = strtok(NULL, ",");
		if (token) d.stock = atoi(token); else d.stock = 0;//??????
        token = strtok(NULL, ",");
		if (token) d.price = atof(token); else d.price = 0.0;//??????
        token = strtok(NULL, ",");
		if (token) strcpy(d.batch, token); else d.batch[0] = '\0';//????????
        token = strtok(NULL, ",");
		if (token) strcpy(d.expiry, token); else d.expiry[0] = '\0';//????§¹????
        token = strtok(NULL, ",");
		if (token) strcpy(d.last_in, token); else d.last_in[0] = '\0';//??????????????????
        token = strtok(NULL, ",");
		if (token) strcpy(d.last_out, token); else d.last_out[0] = '\0';//????????????????????

<<<<<<< HEAD
        // Î²²å·¨¹¹½¨Ò©Æ·Á´±í
=======
        // Î²ï¿½å·¨ï¿½ï¿½ï¿½ï¿½Ò©Æ·ï¿½ï¿½ï¿½ï¿½
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
        Drug* node = (Drug*)malloc(sizeof(Drug));
        *node = d;
        node->next = NULL;
        if (!drugList) drugList = tail = node;
        else { tail->next = node; tail = node; }
    }
    fclose(fp);
}

// ---------------------------------------------------------
<<<<<<< HEAD
// ±£´æ¹ÜÀí¶ËµÄÒ©Æ·Êý¾Ýµ½±¾µØ
=======
// ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ëµï¿½Ò©Æ·ï¿½ï¿½ï¿½Ýµï¿½ï¿½ï¿½ï¿½ï¿?
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
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
// ????????????????????????????????????
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
<<<<<<< HEAD
// ±£´æÒ©Æ·³öÈë¿â±ä¶¯ÀúÊ·¼ÇÂ¼
=======
// ï¿½ï¿½ï¿½ï¿½Ò©Æ·ï¿½ï¿½ï¿½ï¿½ï¿½ä¶¯ï¿½ï¿½Ê·ï¿½ï¿½Â¼
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
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
<<<<<<< HEAD
// ÄÚ²¿¹¤¾ß£º¸ñÊ½»¯´òÓ¡ËùÓÐÒ©Æ·ÐÅÏ¢
// ---------------------------------------------------------
static void displayAllDrugs() {
    if (!drugList) {
        printf("Ò©Æ·¿âÎª¿Õ¡£\n");
=======
// ï¿½Ú²ï¿½ï¿½ï¿½ï¿½ß£ï¿½ï¿½ï¿½Ê½ï¿½ï¿½ï¿½ï¿½Ó¡ï¿½ï¿½ï¿½ï¿½Ò©Æ·ï¿½ï¿½Ï¢
// ---------------------------------------------------------
static void displayAllDrugs() {
    if (!drugList) {
        printf("Ò©Æ·ï¿½ï¿½Îªï¿½Õ¡ï¿½\n");
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
        return;
    }
    printf("\n--- ??????§Ò?? ---\n");
    printf("%-15s %-20s %-10s %-8s %-12s %-12s %-20s %-20s\n", "ID", "????????", "??????", "??????", "????????", "????§¹????", "????????????", "??????????????");
    Drug* p = drugList;
    while (p) {
        printf("%-15s %-20s %-10d %-8.2f %-12s %-12s %-20s %-20s\n",
            p->id, p->name, p->stock, p->price, p->batch, p->expiry,
            p->last_in, p->last_out);
        p = p->next;
    }
}

// ---------------------------------------------------------
<<<<<<< HEAD
// ÒµÎñÒ»£ºÒ©Æ·¿â´æ¶àÎ¬¶È²éÑ¯
// ---------------------------------------------------------
static void drugStockQuery() {
    int choice;
    printf("\nÒ©Æ·¿â´æ²éÑ¯:\n1-°´ID²éÑ¯\n2-°´Ãû³ÆÄ£ºý²éÑ¯\n3-²é¿´ËùÓÐÒ©Æ·\nÇëÑ¡Ôñ: ");
    scanf("%d", &choice);

    // °´¾«È·ID¼ìË÷
=======
// Òµï¿½ï¿½Ò»ï¿½ï¿½Ò©Æ·ï¿½ï¿½ï¿½ï¿½Î¬ï¿½È²ï¿½Ñ¯
// ---------------------------------------------------------
static void drugStockQuery() {
    int choice;
    printf("\nÒ©Æ·ï¿½ï¿½ï¿½ï¿½Ñ¯:\n1-ï¿½ï¿½IDï¿½ï¿½Ñ¯\n2-ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ä£ï¿½ï¿½ï¿½ï¿½Ñ¯\n3-ï¿½é¿´ï¿½ï¿½ï¿½ï¿½Ò©Æ·\nï¿½ï¿½Ñ¡ï¿½ï¿½: ");
    if(scanf("%d", &choice) != 1) {
        choice = -1;
    }

    // ï¿½ï¿½ï¿½ï¿½È·IDï¿½ï¿½ï¿½ï¿½
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
    if (choice == 1) {
        char id[15];
        for(int i=0;i<15;i++)   
			id[i]= '\0'; // ????????????????????¦Â]
        printf("????????????????ID: "); scanf("%s", id);
        if (id[14] != '\0') {
            printf("¦Ä??????????????????\n");
            return ;
        }
        Drug* p = drugList;
        while (p) {
            if (strcmp(p->id,id)==0){
                printf("ID:%s ????????:%s ??????:%d ??????:%.2f ????????:%s ????§¹????:%s\n",
                    p->id, p->name, p->stock, p->price, p->batch, p->expiry);
                return;
            }
            p = p->next;
        }
<<<<<<< HEAD
        printf("Î´ÕÒµ½¸ÃÒ©Æ·¡£\n");
    }
    // °´Ãû³Æ¹Ø¼ü×ÖÄ£ºý¼ìË÷
    else if (choice == 2) {
        char name[50]; printf("ÇëÊäÈëÒ©Æ·Ãû³Æ¹Ø¼ü×Ö: "); scanf("%s", name);
=======
        printf("Î´ï¿½Òµï¿½ï¿½ï¿½Ò©Æ·ï¿½ï¿½\n");
    }
    // ï¿½ï¿½ï¿½ï¿½ï¿½Æ¹Ø¼ï¿½ï¿½ï¿½Ä£ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
    else if (choice == 2) {
        char name[50]; printf("ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ò©Æ·ï¿½ï¿½ï¿½Æ¹Ø¼ï¿½ï¿½ï¿½: "); scanf("%s", name);
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
        int found = 0;
        Drug* p = drugList;
        while (p) {
            if (strstr(p->name, name)) {
<<<<<<< HEAD
                printf("ID:%d Ãû³Æ:%s ¿â´æ:%d ¼Û¸ñ:%.2f ÅúºÅ:%s ÓÐÐ§ÆÚ:%s\n",
=======
                printf("ID:%d ï¿½ï¿½ï¿½ï¿½:%s ï¿½ï¿½ï¿?%d ï¿½Û¸ï¿½:%.2f ï¿½ï¿½ï¿½ï¿½:%s ï¿½ï¿½Ð§ï¿½ï¿½:%s\n",
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
                    p->id, p->name, p->stock, p->price, p->batch, p->expiry);
                found = 1;
            }
            p = p->next;
        }
<<<<<<< HEAD
        if (!found) printf("Î´ÕÒµ½Æ¥ÅäÒ©Æ·¡£\n");
=======
        if (!found) printf("Î´ï¿½Òµï¿½Æ¥ï¿½ï¿½Ò©Æ·ï¿½ï¿½\n");
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
    }
    else if (choice == 3) {
        displayAllDrugs();
    }
<<<<<<< HEAD
    else printf("ÎÞÐ§Ñ¡Ôñ¡£\n");
}

// ---------------------------------------------------------
// ÒµÎñ¶þ£º²é¿´ÀúÊ·¿â´æÁ÷Ë®¼ÇÂ¼
// ---------------------------------------------------------
static void viewStockRecords() {
    printf("\n--- ¿â´æ±ä¶¯¼ÇÂ¼ ---\n");
=======
    else printf("ï¿½ï¿½Ð§Ñ¡ï¿½ï¿½\n");
}

// ---------------------------------------------------------
// Òµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½é¿´ï¿½ï¿½Ê·ï¿½ï¿½ï¿½ï¿½ï¿½Ë®ï¿½ï¿½Â¼
// ---------------------------------------------------------
static void viewStockRecords() {
    printf("\n--- ï¿½ï¿½ï¿½ä¶¯ï¿½ï¿½Â¼ ---\n");
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
    DrugHistory* h = drugHistoryList;
    if (!h) { printf("????????????????\n"); return; }
    printf("%-15s %-6s %-8s %-20s\n", "????ID", "????????", "????????", "??????");
    while (h) {
        printf("%-15s %-6s %-8d %-20s\n", h->drug_id,
            (h->type == 1) ? "??????" : "????????", h->quantity, h->time);
        h = h->next;
    }
}

// ---------------------------------------------------------
<<<<<<< HEAD
// ÒµÎñÈý£ºÒ©Æ·Èë¿â¹ÜÀí (Ôö¼Ó¿â´æ²¢¼ÇÂ¼Á÷Ë®)
=======
// Òµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ò©Æ·ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ (ï¿½ï¿½ï¿½Ó¿ï¿½æ²¢ï¿½ï¿½Â¼ï¿½ï¿½Ë?
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
// ---------------------------------------------------------
static void drugIn() {
    int  quantity;
    char id[15];
    printf("????????????????ID: ");
    id[14] = '\0'; // ????????????????????¦Â
    scanf("%s", id);
    if (id[14]!='\0'){
		printf("¦Ä??????????????????\n");
        return;
    }
    Drug* p = drugList;
    while (p) {
        if (strcmp(p->id,id)==0) {
            printf("????????????: %d\n", p->stock);
            printf("??????????????????????????: ");
            scanf("%d", &quantity);
<<<<<<< HEAD
            if (quantity <= 0) { printf("ÊýÁ¿±ØÐëÎªÕý¡£\n"); return; }

            p->stock += quantity; // Ôö¼ÓÎïÀí¿â´æ
            getCurrentTime(p->last_in, 30); // ¸üÐÂ×î½üÈë¿âÊ±¼ä

            // ²ÉÓÃÍ·²å·¨½«±ä¶¯¼ÇÂ¼²åÈëÀúÊ·Á÷Ë®Á´±í
=======
            if (quantity <= 0) { printf("ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Îªï¿½ï¿½ï¿½ï¿½\n"); return; }

            p->stock += quantity; // ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?
            getCurrentTime(p->last_in, 30); // ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ê±ï¿½ï¿½

            // ï¿½ï¿½ï¿½ï¿½Í·ï¿½å·¨ï¿½ï¿½ï¿½ä¶¯ï¿½ï¿½Â¼ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ê·ï¿½ï¿½Ë®ï¿½ï¿½ï¿½ï¿½
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
            DrugHistory* h = (DrugHistory*)malloc(sizeof(DrugHistory));
            strcpy(h->drug_id,id);
            h->type = 1;
            h->quantity = quantity;
            getCurrentTime(h->time, 30);
            h->next = drugHistoryList;
            drugHistoryList = h;
<<<<<<< HEAD
            printf("Èë¿â³É¹¦£¬ÐÂ¿â´æ: %d\n", p->stock);
=======
            printf("ï¿½ï¿½ï¿½É¹ï¿½ï¿½ï¿½ï¿½Â¿ï¿½ï¿? %d\n", p->stock);
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
            return;
        }
        p = p->next;
    }
<<<<<<< HEAD
    printf("Î´ÕÒµ½¸ÃÒ©Æ·¡£\n");
}

// ---------------------------------------------------------
// ÒµÎñËÄ£ºÒ©Æ·ÈË¹¤³ö¿â¹ÜÀí
=======
    printf("Î´ï¿½Òµï¿½ï¿½ï¿½Ò©Æ·ï¿½ï¿½\n");
}

// ---------------------------------------------------------
// Òµï¿½ï¿½ï¿½Ä£ï¿½Ò©Æ·ï¿½Ë¹ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
// ---------------------------------------------------------
static void drugOut() {
    int  quantity;
    char id[15];
    printf("????????????????ID: ");
    id[14] = '\0'; // ????????????????????¦Â
    scanf("%s", id);
    if (id[14] != '\0') {
        printf("¦Ä??????????????????\n");
        return;
	}//????????????????????????????????????????
    Drug* p = drugList;
    while (p) {
        if (strcmp(p->id,id)==0) {
            printf("????????????: %d\n", p->stock);
            printf("??????????????????????????: ");
            scanf("%d", &quantity);
<<<<<<< HEAD
            if (quantity <= 0) { printf("ÊýÁ¿±ØÐëÎªÕý¡£\n"); return; }
            if (p->stock < quantity) { printf("¿â´æ²»×ã£¡\n"); return; }

            p->stock -= quantity; // ¿Û¼õÎïÀí¿â´æ
            getCurrentTime(p->last_out, 30);

            // Éú³ÉÁ÷Ë®¼ÇÂ¼
=======
            if (quantity <= 0) { printf("ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Îªï¿½ï¿½ï¿½ï¿½\n"); return; }
            if (p->stock < quantity) { printf("ï¿½ï¿½æ²»ï¿½ã£¡\n"); return; }

            p->stock -= quantity; // ï¿½Û¼ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?
            getCurrentTime(p->last_out, 30);

            // ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ë®ï¿½ï¿½Â¼
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
            DrugHistory* h = (DrugHistory*)malloc(sizeof(DrugHistory));
            strcpy(h->drug_id ,id);
            h->type = 2; // 2????????????????
            h->quantity = quantity;
            getCurrentTime(h->time, 30);
            h->next = drugHistoryList;
            drugHistoryList = h;
<<<<<<< HEAD
            printf("³ö¿â³É¹¦£¬ÐÂ¿â´æ: %d\n", p->stock);
=======
            printf("ï¿½ï¿½ï¿½ï¿½É¹ï¿½ï¿½ï¿½ï¿½Â¿ï¿½ï¿½: %d\n", p->stock);
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
            return;
        }
        p = p->next;
    }
<<<<<<< HEAD
    printf("Î´ÕÒµ½¸ÃÒ©Æ·¡£\n");
=======
    printf("Î´ï¿½Òµï¿½ï¿½ï¿½Ò©Æ·ï¿½ï¿½\n");
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
}
// ---------------------------------------------------------
// ????????‰Ø??????????????????????
//----------------------------------------------------------
void addDrug() {
    Drug d;
    printf("????????????????????ID: ");
    d.id[14] = '\0'; // ????????????????????¦Â
    scanf("%s", d.id);
    if (d.id[14] != '\0') {
        printf("????ID????????????????????????????\n");
        return;
    }
<<<<<<< HEAD
    printf("????????????????????????: ");
    scanf("%s", d.name);
    printf("??????????????????????????????: ");
    scanf("%d", &d.stock);
    printf("??????????????????????: ");
    scanf("%f", &d.price);
    printf("????????????????????: ");
    scanf("%s", d.batch);
    printf("????????????????§¹???? (????2025-12-31): ");
    scanf("%s", d.expiry);
    getCurrentTime(d.last_in, 30); // ????????????????????????
    strcpy(d.last_out, ""); // ??????????????????????????
    // ????????????????????????????????¦Â
=======
    printf("ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ò©Æ·ï¿½ï¿½ï¿½ï¿½: ");
    scanf("%20s", d.name);
    printf("ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ê¼ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½: ");
    while (1) {
        if (scanf("%d", &d.stock) == 1)
            break;
        while (getchar() != '\n');
           printf("ï¿½ï¿½ï¿½ï¿½Ä²ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ö£ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿? ");
    }
    printf("ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ò©Æ·ï¿½Û¸ï¿½: ");
    while (1) {
        if (scanf("%f", &d.price));
            break;
        while (getchar() != '\n');
        printf("ï¿½ï¿½ï¿½ï¿½Ä²ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ö£ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿? ");
    }
    printf("ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½: ");
    scanf("%s", d.batch);
    printf("ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ð§ï¿½ï¿½ (ï¿½ï¿½XXXX-YY-ZZ): ");
	judgetime(d.expiry);// ï¿½ï¿½Ö¤ï¿½ï¿½ï¿½Ú¸ï¿½Ê½
    getCurrentTime(d.last_in, 30); // ï¿½ï¿½Ê¼ï¿½ï¿½ï¿½Ê±ï¿½ï¿½Îªï¿½ï¿?
    strcpy(d.last_out, ""); // ï¿½ï¿½Ê¼ï¿½ï¿½ï¿½ï¿½Ê±ï¿½ï¿½Îªï¿½ï¿½
    // ï¿½ï¿½ï¿½ï¿½Ò©Æ·ï¿½ï¿½ï¿½Óµï¿½ï¿½ï¿½ï¿½ï¿½Ä©Î²
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
    Drug* node = (Drug*)malloc(sizeof(Drug));
    *node = d;
    node->next = NULL;
    if (!drugList) drugList = node;
    else {
        Drug* p = drugList;
        while (p->next) p = p->next;
        p->next = node;
    }
    printf("????????????????????????\n");
}

// ---------------------------------------------------------
<<<<<<< HEAD
// ¹ÜÀí¶Ë£ºÒ©·¿ÏµÍ³×ÓÂ·ÓÉ
=======
// ï¿½ï¿½ï¿½ï¿½ï¿½Ë£ï¿½Ò©ï¿½ï¿½ÏµÍ³ï¿½ï¿½Â·ï¿½ï¿½
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
// ---------------------------------------------------------
void drugMenu() {
    int choice;
    do {
<<<<<<< HEAD
        // ??????????????????????????????????????????????????
        printf("\n========== ?????????????? ==========\n");
        printf("1. ??????????????\n");
        printf("2. ??????????\n");
        printf("3. ????????????\n");
        printf("4. ??????????????????\n");
		printf("5.??????????????????\n");
        printf("0. ??????????????????\n");
        printf("??????????: ");
        scanf("%d", &choice);
=======
        // ×¢ï¿½â£ºï¿½ï¿½ï¿½ï¿½ï¿½Â¼ï¿½ï¿½ï¿½Ø£ï¿½Ö±ï¿½Ó³ï¿½ï¿½ï¿½Òµï¿½ï¿½Ëµï¿½
        printf("\n========== Ò©ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ==========\n");
        printf("1. Ò©Æ·ï¿½ï¿½ï¿½ï¿½Ñ¯\n");
        printf("2. Ò©Æ·ï¿½ï¿½ï¿½\n");
        printf("3. Ò©Æ·ï¿½ï¿½ï¿½ï¿½\n");
        printf("4. ï¿½é¿´ï¿½ï¿½ï¿½ä¶¯ï¿½ï¿½Â¼\n");
		printf("5.ï¿½ï¿½ï¿½ï¿½Ò©Æ·ï¿½ï¿½Ï¢\n");
        printf("0. ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ëµï¿½\n");
        printf("ï¿½ï¿½Ñ¡ï¿½ï¿½: ");
        if (scanf("%d", &choice) != 1)
            choice = -1;
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
        switch (choice) {
        case 1: drugStockQuery(); break;
        case 2: drugIn(); break;
        case 3: drugOut(); break;
        case 4: viewStockRecords(); break;
		case 5: addDrug(); break;
        case 0: break;
<<<<<<< HEAD
        default: printf("ÎÞÐ§Ñ¡Ïî¡£\n");
=======
        default: printf("ï¿½ï¿½Ð§Ñ¡ï¿½î¡£\n");
>>>>>>> 84ae28902b24ca7d8c8b7d99bcdcb8609ec720f2
        }
    } while (choice != 0);
}