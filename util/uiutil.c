#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "uiutil.h"
#include "keylistener.h"
#include "texts.h"

String* receiveStringDialog(char *msg, int maxLen) {
    StringBuilder *sb = newStringBuilder(sb);
    appendSpaceStringBuilder(sb);
    appendMStringBuilder(sb, 4, SIMPLE_LINE, msg, "\n", SIMPLE_LINE);
    printf(sb->string);
    String *input = NULL;
    while(1) {
        input = readline();
        if(countUtf8Char(input) < maxLen) break;
        freeString(input);
        sb->size = 0;
        appendIntStringBuilder(sb, maxLen);
        appendStringBuilder(sb, "자 미만으로 입력해주세요.\n");
        printf(sb->string);
    }
    freeStringBuilder(sb);
    df1("RECEIVED: [%s]\n", input->content);
    return input;
}

void dialog(char *msg) {
    StringBuilder *sb = newStringBuilder();
    appendSpaceStringBuilder(sb);
    appendStringBuilder(sb, SIMPLE_LINE);
    appendlnStringBuilder(sb);
    appendStringBuilder(sb, msg);
    appendlnStringBuilder(sb);
    appendlnStringBuilder(sb);
    appendStringBuilder(sb, SIMPLE_LINE);
    appendStringBuilder(sb, "계속하려면 아무 키나 눌러주세요.\n");
    appendStringBuilder(sb, SIMPLE_LINE);
    printf(sb->string);
    freeStringBuilder(sb);
    mygetch();
}
int boolDialog(char *msg) {
    int select = 0;
    StringBuilder *sb = newStringBuilder();
    while(1) {
        sb->size = 0;
        appendSpaceStringBuilder(sb);
        appendStringBuilder(sb, SIMPLE_LINE);
        appendlnStringBuilder(sb);
        appendStringBuilder(sb, msg);
        appendlnStringBuilder(sb);
        appendlnStringBuilder(sb);
        appendStringBuilder(sb, SIMPLE_LINE);
        appendlnStringBuilder(sb);
        if(select == 0) appendStringBuilder(sb, "▶ ");
        appendStringBuilder(sb, "아니요\n");
        appendlnStringBuilder(sb);
        if(select == 1) appendStringBuilder(sb, "▶ ");
        appendStringBuilder(sb, "네\n");
        appendlnStringBuilder(sb);
        appendStringBuilder(sb, SIMPLE_LINE);
        appendStringBuilder(sb, "⬆⬇    선택\nEnter    확인\n");
        appendStringBuilder(sb, SIMPLE_LINE);
        printf(sb->string);
        int key = mygetch();
        if(key == KEYS.arrowDown || key == KEYS.arrowUp) {
            select = !select;
            continue;
        }
        if(key == KEYS.enter) break;
    }
    freeStringBuilder(sb);
    return select;
}
void clearInputBuffer() {
    fflush(stdin);
}

void inputDialog(char* msg, char* target, int lengthLimit) {
    clearInputBuffer();
    StringBuilder *sb = newStringBuilder();
    appendSpaceStringBuilder(sb);
    appendStringBuilder(sb, SIMPLE_LINE);
    appendStringBuilder(sb, msg);
    appendStringBuilder(sb, SIMPLE_LINE);
    appendlnStringBuilder(sb);
    printf(sb->string);
    freeStringBuilder(sb);
    char input[1024];
    mygets(input, sizeof(input));
    input[strcspn(input, "\n")] = '\0';
    int length = strlen(input);
    if(length > lengthLimit-1) {
        dialog("입력 값이 너무 깁니다!");
        return;
    }
    for(int i = 0; i<length+1; i++) target[i] = input[i];
}



int boundaryMove(int current, int max, int delta) {
    if(max == 0) return 0;
    current += delta;
    if(current < 0) current += max;
    return current % max;
}
