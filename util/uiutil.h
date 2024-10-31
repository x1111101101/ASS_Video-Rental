#ifndef UIUTIL_H
#define UIUTIL_H

#include <stdio.h>
#include "texts.h"

#define SIMPLE_LINE "➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖➖\n"

void dialog(char* msg);
int boolDialog(char *msg);
void inputDialog(char* msg, char* target, int lengthLimit);
void clearInputBuffer();
String* receiveStringDialog(char *msg, int maxLen);

int boundaryMove(int current, int max, int delta);

#define TOOLTIP_CHOICE              "⬆⬇         선택\n"
#define TOOLTIP_PAGE                "◁ ▷        다음/이전 페이지\n"
#define TOOLTIP_CONFIRM             "Enter      확인\n"
#define TOOLTIP_EDIT                "Enter      편집\n"
#define TOOLTIP_CONFIRM_OR_EDIT     "Enter      확인, 편집\n"
#define TOOLTIP_QUERY               "Q          검색\n"
#define TOOLTIP_EXIT                "ESC        나가기\n"
#define TOOLTIP_CREATE              "C          생성\n"

#endif