#include <stdio.h>
#include <stdlib.h>
#include "db.h"
#include "user.h"
#include "service.h"
#include "screen\screen.h"
#include "util\texts.h"
#include "util\uiutil.h"
#include "util\keylistener.h"
#include "util\mystructures.h"

void initProgram();

int main(void) {
    initProgram();
    initMygetch();
    printf("프로그램을 시작합니다.\n");
    Service *service = loadService();
    initService(service);
    mainScreen(service);
}

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <locale.h>
/**
 * Window OS
 */
void initProgram() {
    //printf("initializing for windows\n");
    SetConsoleOutputCP(65001); // 콘솔 출력 인코딩을 UTF-8로 설정
}
#else
/**
 * Linux OS
 */
#include <locale.h>
void initProgram() {
    printf("initializing for linux\n");
    setlocale(LC_ALL, "");
}
#endif