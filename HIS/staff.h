#pragma once
#ifndef STAFF_H
#define STAFF_H
#include "models.h"

// 向 main 暴露医护端总路由，接收 main 传入的登录医生指针
void staffTerminal(Staff* me);

#endif