#include "keylistener.h"
#include <stdio.h>

KeySet KEYS;

void initKeys() {
    KeySet keyset;
    keyset.arrowLeft = 75;
    keyset.arrowRight = 77;
    keyset.arrowUp = 72;
    keyset.arrowDown = 80;
    keyset.lowercaseQ = 113;
    keyset.uppercaseQ = 81;
    keyset.lowercaseC = 99;
    keyset.uppercaseC = 67;
    keyset.enter = 13;
    keyset.esc = 27;
    keyset.lowercaseE = 101;
    keyset.uppercaseE = 69;
    keyset.spacebar = 32;
    KEYS = keyset;
}


#if defined(_WIN32) || defined(_WIN64)
/**
 * Window OS
 */

#include <conio.h>

int mygetch() {
    return getch();
}
void initMygetch() {
    initKeys();
}
#else
/**
 * Linux OS
 */
#include <termios.h>
#include <unistd.h>

void initMygetch() {
    initKeys();
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);      // 현재 터미널 설정 읽기
    t.c_lflag &= ~(ICANON | ECHO);    // 캐논 모드와 에코 비활성화
    tcsetattr(STDIN_FILENO, TCSANOW, &t); // 변경 사항 즉시 적용
}

int mygetch() {
    return getchar();
}

#endif

