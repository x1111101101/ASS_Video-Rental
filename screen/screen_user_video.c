#include "screen.h"
#include <stdio.h>
#include "../util/texts.h"
#include "../util/uiutil.h"
#include "../util/debug.h"
#include "../util/math.h"
#include "../util/mystructures.h"
#include "../util/keylistener.h"
#include "../service.h"
#include "../db.h"
#include "../video.h"
#include "../article.h"
#include "../user.h"


#define STOCK_ACTION_EXIT  (0)
#define STOCK_ACTION_QUERY  (1)
#define STOCK_ACTION_REFRESH  (2)

static int _userStockScreen(Service *service, Article *article, User *user, ArrayList *list, char *title) {
    StringBuilder *sb = newStringBuilder();
    static int page = 0;
    int perPage = 10;
    int option = 0;
    int action = STOCK_ACTION_EXIT;
    char buf[1024];
    while(1) {
        sb->size = 0;
        sprintf(buf, "%-27s %-27s %-27s\n", "[식별번호]", "[입고일]", "[대여현황]");
        appendSpaceStringBuilder(sb);
        appendMStringBuilder(sb, 5, SIMPLE_LINE, title, "\n\n", buf, SIMPLE_LINE);
        int pageSize = (list->size + perPage-1) / perPage;
        page = fitRange(page, 0, pageSize-1);
        int start = page*perPage;
        int end = MIN(list->size, start + perPage);
        int size = end-start;
        option = fitRange(option, 0, size-1);
        if(size == 0) appendStringBuilder(sb, "페이지가 비어있습니다.\n");
        else for(int i = 0; i<size; i++) {
            if(option == i) appendStringBuilder(sb, "▶ ");
            Video *video = (Video*) list->arr[start+i];
            String *storedDateString = toStringDate(&video->storedDate);
            static char* rentFlags[] = {"", "대여중"};
            char *rentFlag = ((video->rentalId) == -1) ? rentFlags[0] : rentFlags[1];
            sprintf(buf, "%-27s %-27s %-27s", video->videoId->content, storedDateString->content, rentFlag);
            printf("%d\n", video->articleId);
            freeString(storedDateString);
            appendMStringBuilder(sb, 2, buf, "\n");
        }
        appendStringBuilder(sb, SIMPLE_LINE);
        appendMStringBuilder(sb, 6, TOOLTIP_CHOICE, TOOLTIP_PAGE, TOOLTIP_CONFIRM, TOOLTIP_QUERY, TOOLTIP_EXIT, SIMPLE_LINE);
        printf(sb->string);
        int key = mygetch();
        if(key == KEYS.esc) { // Exit
            page = 0;
            break;
        }
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
            Video *video = list->arr[start+option];
            sprintf(buf, "%s 작품의 비디오 %s을(를) 대여하시겠습니까?", article->name->content, video->videoId->content);
            if(!boolDialog(buf)) continue;
            Response res = rentVideoService(service, user->id, video->id);
            if(res.succeed) {
                dialog("대여를 완료하였습니다.");
                action = STOCK_ACTION_EXIT;
                freeString(res.msg);
                break;
            }
            dialog(res.msg->content);
            freeString(res.msg);
            continue;
        }
        if(key == KEYS.lowercaseQ || key == KEYS.uppercaseQ) { // Query
            action = STOCK_ACTION_QUERY;
            break;
        }
    }
    freeStringBuilder(sb);
    return action;
}

static void _filterRentable(ArrayList *list) {
    int k = 0;
    for(int i = 0; i<list->size; i++) {
        Video *video = (Video*) getArrayList(list, i);
        if(video->rentalId != -1) continue;
        list->arr[k++] = video;
    }
    list->size = k;
}

void userVideoListByArticleScreen(Service *service, User *user, Article *article) {
    int result = STOCK_ACTION_EXIT;
    while(1) {
        ArrayList *list = getVideosService(service, article->id);
        _filterRentable(list);
        result = _userStockScreen(service, article, user, list, "대여 가능 재고 목록");
        freeArrayList(list);
        if(result == STOCK_ACTION_EXIT) break;
        while(result == STOCK_ACTION_QUERY) {
            String *query = receiveStringDialog("재고 식별 번호 검색어를 입력해주세요.", 30);
            ArrayList *queryResult = queryVideosService(service, query);
            freeString(query);
            int k = 0;
            for(int i = 0; i<queryResult->size; i++) {
                Video *video = (Video*) getArrayList(queryResult, i);
                if(video->articleId != article->id) continue;
                queryResult->arr[k++] = video;
            }
            queryResult->size = k;
            _filterRentable(queryResult);
            result = _userStockScreen(service, article, user, queryResult, "재고 검색 결과");
            freeArrayList(queryResult);
        }
    }
}