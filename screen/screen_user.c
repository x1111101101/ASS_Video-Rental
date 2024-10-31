#include <stdio.h>
#include "../util/texts.h"
#include "../util/uiutil.h"
#include "../util/mystructures.h"
#include "../util/keylistener.h"
#include "../service.h"
#include "../db.h"
#include "../video.h"
#include "../article.h"
#include "../user.h"
#include "screen.h"

void userMainScreen(Service *service) {
    User *user = service->loginUser;
    StringBuilder *sb = newStringBuilder(sb);
    int option = 0;
    int optionSize = 12;
    char *optionNames[12] = {"모든 작품"};
    for(int i = 0; i<=ECATEGORY_ETC; i++) {
        optionNames[i+1] = CATEGORY_NAMES[i];
    }
    optionNames[CATEGORY_SIZE+1] = "대여 기록";
    optionNames[CATEGORY_SIZE+2] = "반납하기";
    optionNames[CATEGORY_SIZE+3] = "회원 정보";
    while(1) {
        sb->size = 0;
        appendSpaceStringBuilder(sb);
        appendMStringBuilder(sb, 3, SIMPLE_LINE, "\n비디오 대여\n\n", SIMPLE_LINE);
        for(int i = 0; i<=CATEGORY_SIZE; i++) {
            if(option == i) appendStringBuilder(sb, "▶ ");
            appendMStringBuilder(sb, 2, optionNames[i], "\n");
        }
        appendStringBuilder(sb, "\n");
        for(int i = CATEGORY_SIZE+1; i < optionSize; i++) {
            if(option == i) appendStringBuilder(sb, "▶ ");
            appendMStringBuilder(sb, 2, optionNames[i], "\n");
        }
        printf(sb->string);
        int key = mygetch();
        if(key == KEYS.esc) {
            if(!boolDialog("정말 로그아웃하시겠습니까?")) continue;
            logoutService(service);
            break;
        }
        if(key == KEYS.arrowDown || key == KEYS.arrowUp) {
            int delta = 1 - 2*(key == KEYS.arrowUp);
            option = boundaryMove(option, optionSize, delta);
            continue;
        }
        if(key != KEYS.enter) continue;
        if(option == 0) {
            userArticleListScreen(service, user);
            continue;
        }
        if(option <= CATEGORY_SIZE) {
            userArticleListByCategoryScreen(service, user, (Category) option-1);
            continue;
        }
        int relative = option - CATEGORY_SIZE - 1;
        if(relative == 0) {
            rentalListScreen(service, user);
            continue;
        }
        if(relative == 1) {
            returnScreen(service, user);
            continue;
        }
        userInfoScreen(service, user);
    }
    freeStringBuilder(sb);
}

void userInfoScreen(Service *service, User *user) {
    StringBuilder *sb = newStringBuilder();
    char buf[1024];
    while(1) {
        sb->size = 0;
        appendSpaceStringBuilder(sb);
        appendMStringBuilder(sb, 3, SIMPLE_LINE, "회원 정보 관리\n", SIMPLE_LINE);
        appendMStringBuilder(sb, 3, "이름: ", user->name->content, "\n");
        appendMStringBuilder(sb, 3, "연락처: ", user->phone->content, "\n");
        appendMStringBuilder(sb, 3, "아이디: ", user->loginId->content, "\n");
        Date birth = user->birthday;
        String *birthStr = toStringDate(&birth);
        appendMStringBuilder(sb, 4, "생년월일: ", birthStr->content, "\n", SIMPLE_LINE);
        freeString(birthStr);
        appendMStringBuilder(sb, 3, "비밀번호 외 개인정보 수정은 관리자 권한입니다.\n"
            , "▶ [비밀번호 변경하기]\n", SIMPLE_LINE);
        appendMStringBuilder(sb, 3, TOOLTIP_CONFIRM, TOOLTIP_EXIT, SIMPLE_LINE);
        printf(sb->string);
        int key = mygetch();
        if(key == KEYS.esc) break;
        if(key != KEYS.enter) continue;
        if(!boolDialog("비밀번호를 변경하시겠습니까?")) continue;
        String *newPassword = receiveStringDialog("변경할 비밀번호를 입력해주세요.", 30);
        if(countUtf8Char(newPassword) < 8) {
            dialog("비밀번호를 8자리 이상의 문자열이여야합니다.");
        } else {
            changeUserPasswordService(service, user, newPassword);
            dialog("비밀번호를 변경하였습니다.");
        }
        freeString(newPassword);
    }
    freeStringBuilder(sb);
}