#pragma once
#ifndef FILEIO_H
#define FILEIO_H

// 1. 全局基础数据读写 
void loadAllDataFromTxt();
void saveAllDataToTxt();

// 2. 药品与库存读写 
void loadDrugs();
void saveDrugs();
void loadDrugHistory();
void saveDrugHistory();


// 4. 管理员数据读写 
void loadAdminData();
void saveAdminData();

// 5. 排班与流水读写 
void loadSchedules();
void saveSchedules();
void loadTransactions();
void saveTransactions();

#endif
