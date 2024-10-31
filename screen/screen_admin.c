#include <stdio.h>
#include "../util/texts.h"
#include "../util/uiutil.h"
#include "../util/mystructures.h"
#include "../util/keylistener.h"
#include "../util/math.h"
#include "../service.h"
#include "../db.h"
#include "../video.h"
#include "../article.h"
#include "../user.h"
#include "screen.h"

static void manageRentalScreen(Service *service) {
    int action = RENTAL_ACTION_EXIT;
    while(1) {
        ArrayList *list = getAllRentalsService(service);
        sortArrayList(list, RENTAL_ID_COMPARATOR);
        action = rentalAdminScreen(service, list);
        freeArrayList(list);
        if(action == RENTAL_ACTION_EXIT) break;
    }
}

void adminMainScreen(Service *service) {
    StringBuilder *sb = newStringBuilder(sb);
    int option = 0;
    int optionSize = 3;
    char optionNames[][24] = {"사용자 관리", "작품 관리", "대여 관리"};
    while(1) {
        sb->size = 0;
        appendSpaceStringBuilder(sb);
        appendMStringBuilder(sb, 4, SIMPLE_LINE, "\n비디오 대여 - 관리자 모드\n\n", SIMPLE_LINE, "\n");
        for(int i = 0; i<optionSize; i++) {
            if(option == i) appendStringBuilder(sb,  "▶ ");
            appendMStringBuilder(sb, 2, optionNames[i], "\n\n");
        }
        appendStringBuilder(sb, SIMPLE_LINE);
        appendMStringBuilder(sb, 4, TOOLTIP_CHOICE, TOOLTIP_CONFIRM, TOOLTIP_EXIT, SIMPLE_LINE);
        printf(sb->string);
        int key = mygetch();
        if(key == KEYS.arrowDown || key == KEYS.arrowUp) {
            int delta = 1 - 2*(key == KEYS.arrowUp);
            option = boundaryMove(option, optionSize, delta);
            continue;
        }
        if(key == KEYS.esc) {
            if(boolDialog("로그아웃하시겠습니까?")) {
                logoutService(service);
                break;
            }
            continue;
        }
        if(key != KEYS.enter) continue;
        if(option == 0) {
            userManageScreen(service);
            continue;
        }
        if(option == 1) {
            articleManageScreen(service);
            continue;
        }
        if(option == 2) {
            manageRentalScreen(service);
        }
    }
    freeStringBuilder(sb);
}