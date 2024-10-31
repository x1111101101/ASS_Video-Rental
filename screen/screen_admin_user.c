#include <stdio.h>
#include "../util/texts.h"
#include "../util/uiutil.h"
#include "../util/mystructures.h"
#include "../util/keylistener.h"
#include "../util/debug.h"
#include "../util/math.h"
#include "../service.h"
#include "../db.h"
#include "../video.h"
#include "../article.h"
#include "../user.h"
#include "screen.h"

#define USER_LIST_ACTION_EXIT   (0)
#define USER_LIST_ACTION_QUERY   (1)
#define USER_DETAIL_ACTION_REMOVED   (2)

static void _userRentalListScreen(Service *service, User *user) {
    while(1) {
        ArrayList *list = getRentalsByUserService(service, user->id);
        sortArrayList(list, RENTAL_ID_COMPARATOR);
        d("sorted");
        int result = rentalAdminScreen(service, list);
        freeArrayList(list);
        if(result == RENTAL_ACTION_EXIT) break;
    }
}

int _userDetailScreen(Service *service, User *user) {
    int result = 0;
    int userId = user->id;
    StringBuilder *sb = newStringBuilder(sb);
    int options = 3;
    int option = 0;
    char *optionNames[] = {"사용자 정보 수정", "대여 관리", "사용자 삭제"};
    while(1) {
        sb->size = 0;
        appendSpaceStringBuilder(sb);
        appendMStringBuilder(sb, 5, SIMPLE_LINE, "사용자 관리 - ", user->name->content, "\n", SIMPLE_LINE);
        appendMStringBuilder(sb, 3, "이름: ", user->name->content, "\n");
        appendMStringBuilder(sb, 3, "아이디: ", user->loginId->content, "\n");
        String *dateStr = toStringDate(&user->birthday);
        appendMStringBuilder(sb, 3, "생년월일: ", dateStr->content, "\n");
        freeString(dateStr);
        appendMStringBuilder(sb, 3, "연락처: ", user->phone->content, "\n");
        appendStringBuilder(sb, SIMPLE_LINE);
        for(int i = 0; i<options; i++) {
            if(option == i) appendStringBuilder(sb, "▶ ");
            appendMStringBuilder(sb, 2, optionNames[i], "\n");
        }
        appendMStringBuilder(sb, 5, SIMPLE_LINE, TOOLTIP_CHOICE, TOOLTIP_CONFIRM, TOOLTIP_EXIT, SIMPLE_LINE);
        printf(sb->string);
        int key = mygetch();
        if(key == KEYS.esc) break;
        if(key == KEYS.arrowDown || key == KEYS.arrowUp) { // Choice
            int delta = 1 - 2*(key == KEYS.arrowUp);
            option = boundaryMove(option, options, delta);
            continue;
        }
        if(key != KEYS.enter) continue;
        if(option == 0) { // 편집
            userEditScreen(service, user);
            user = getUserByIdService(service, userId);
            result = 1;
            d("end of edit");
            continue;
        }
        if(option == 1) { // 대여 관리
            _userRentalListScreen(service, user);
            continue;
        }
        if(option == 2) { // 삭제
            ArrayList *rentals = getRentalsByUserService(service, userId);
            if(rentals->size > 0) {
                if(!boolDialog("사용자 삭제 시 사용자의 모든 대여 비디오들은 반납 처리되며, 사용자의 대여 기록은 삭제됩니다. \n삭제하시겠습니까?"))
                    continue;
            }
            freeArrayList(rentals);
            Response res = removeUserService(service, userId);
            if(!res.succeed) {
                dialog(res.msg->content);
                freeString(res.msg);
            } else {
                result = 1;
                freeString(res.msg);
                break;
            }
            continue;
        }
    }
    freeStringBuilder(sb);
}

int _userListScreen(Service *service, ArrayList *list, char *title) {
    StringBuilder *sb = newStringBuilder(sb);
    int page = 0;
    int perPage = 10;
    int option = 0;
    int action = USER_LIST_ACTION_EXIT;
    char buf[1024];
    while(1) {
        sb->size = 0;
        sprintf(buf, "%-17s %-17s %-17s\n", "[이름]", "[아이디]", "[연락처]");
        appendSpaceStringBuilder(sb);
        appendMStringBuilder(sb, 5, SIMPLE_LINE, title, "\n\n", buf, SIMPLE_LINE);
        int pageSize = (list->size + perPage-1) / perPage;
        page = fitRange(page, 0, pageSize-1);
        int start = page*perPage;
        int end = MIN(list->size, start + perPage);
        int size = end-start;
        option = fitRange(option, 0, size-1);
        df3("size: %d, page: %d, option: %d\n", size, page, option);
        df1("start: %d\n", start);
        if(size == 0) appendStringBuilder(sb, "페이지가 비어있습니다.\n");
        else for(int i = 0; i<size; i++) {
            df1("USER: %p\n", list->arr[start+i]);
            if(option == i) appendStringBuilder(sb, "▶ ");
            User *user = (User*) list->arr[start+i];
            sprintf(buf, "%-17s %-17s %-17s", user->name->content, user->loginId->content, user->phone->content);
            appendMStringBuilder(sb, 2, buf, "\n");
        }
        appendStringBuilder(sb, SIMPLE_LINE);
        appendMStringBuilder(sb, 6, TOOLTIP_CHOICE, TOOLTIP_PAGE, TOOLTIP_CONFIRM, TOOLTIP_QUERY, TOOLTIP_EXIT, SIMPLE_LINE);
        printf(sb->string);
        int key = mygetch();
        if(key == KEYS.esc) break; // Exit
        if(key == KEYS.arrowDown || key == KEYS.arrowUp) { // Choice
            int delta = 1 - 2*(key == KEYS.arrowUp);
            option = boundaryMove(option, size, delta);
            continue;
        }
        if(key == KEYS.arrowLeft || key == KEYS.arrowRight) { // Change Page
            int delta = 1 - 2*(key == KEYS.arrowLeft);
            page = boundaryMove(page, pageSize, delta);
            continue;
        }
        if(key == KEYS.enter) { // Confirm
            if(size == 0) continue;
            
            User *user = list->arr[start+option];
            if(_userDetailScreen(service, user)) {
                action = 100;
                break;
            }
            continue;
        }
        if(key == KEYS.lowercaseQ || key == KEYS.uppercaseQ) { // Query
            action = USER_LIST_ACTION_QUERY;
            break;
        }
    }
    freeStringBuilder(sb);
    return action;
}

/**
 * Stack Overflow를 방지하기 위해 함수의 중첩을 최소화 하도록 반복문 기반 구현
 * 검색화면 -> 검색화면의 중첩을 방지.
 */
void userManageScreen(Service *service) {
    while(1) {
        ArrayList *users = getAllUsersService(service);
        int action = _userListScreen(service, users, "사용자 관리");
        if(action == USER_LIST_ACTION_EXIT) {
            freeArrayList(users);
            break;
        }
        while(action == USER_LIST_ACTION_QUERY) {
            String *query = receiveStringDialog("검색어를 입력해주세요. (이름, 연락처 기준 검색)\n초성 검색: 홍길동->ㅎㄱㄷ", 30);
            ArrayList *queryResult = queryUserService(service, query);
            StringBuilder *sb = newStringBuilder();
            appendMStringBuilder(sb, 3, "사용자 검색: '", query->content, "'");
            freeString(query);
            action = _userListScreen(service, queryResult, sb->string);
            freeStringBuilder(sb);
            freeArrayList(queryResult);
        }
    }
}
