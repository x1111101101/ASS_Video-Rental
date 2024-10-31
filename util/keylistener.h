/**
 * keylistener.h
 * 콘솔 입력 발생 시 버퍼링 없이 즉시 불러올 때 활용
 */
#ifndef KL_H
#define KL_H

typedef struct KeySet {
    int arrowLeft, arrowRight, arrowUp, arrowDown,
        lowercaseQ, uppercaseQ, lowercaseC, uppercaseC,
        lowercaseE, uppercaseE,
        enter, esc, spacebar;
} KeySet;

KeySet KEYS;

int mygetch();
void initMygetch();

#endif