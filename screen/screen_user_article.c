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
#include "screen.h"

#define ARTICLE_ACTION_EXIT (0)
#define ARTICLE_ACTION_QUERY (1)

static void _articleDetailScreen(Service *service, User *user, Article *article) {
    char buf[1024];
    char *rentFlagNames[] = {"[대여하기]", "[반납하기]"};
    StringBuilder *sb = newStringBuilder();
    while(1) {
        Rental *rent = getCurrentRentalByArticleAndUser(service, article->id, user->id);
        int isRenting = rent != NULL;
        sb->size = 0;
        appendSpaceStringBuilder(sb);
        appendMStringBuilder(sb, 3, SIMPLE_LINE, "작품 정보\n", SIMPLE_LINE);
        appendMStringBuilder(sb, 3, "제목: ", article->name->content, "\n");
        appendMStringBuilder(sb, 3, "작품 설명: ", article->description->content, "\n");
        appendMStringBuilder(sb, 3, "작품 장르: ", CATEGORY_NAMES[article->category], "\n");
        appendMStringBuilder(sb, 4, "시청 등급: ", RATING_NAMES[article->rating], "\n", SIMPLE_LINE);
        VideoStock stock = getVideoStockService(service, article->id);
        sprintf(buf, "재고: %d / %d\n", stock.left, stock.total);
        appendStringBuilder(sb, buf);
        appendMStringBuilder(sb, 3, "▶", rentFlagNames[isRenting], "\n");
        appendMStringBuilder(sb, 5, SIMPLE_LINE, TOOLTIP_CHOICE, TOOLTIP_CONFIRM, TOOLTIP_EXIT, SIMPLE_LINE);
        printf(sb->string);
        int key = mygetch();
        if(key == KEYS.esc) break;
        if(key != KEYS.enter) continue;
        if(isRenting) {
            returnVideoService(service, rent);
            dialog("비디오를 정상적으로 반납하였습니다.");
            continue;
        }
        if(stock.left == 0) {
            dialog("대여 가능한 재고가 없습니다.");
            continue;
        }
        userVideoListByArticleScreen(service, user, article);
    }
    freeStringBuilder(sb);
}

static int _articleListScreen(Service *service, User *user, ArrayList *list, char *title) {
    StringBuilder *sb = newStringBuilder(sb);
    int page = 0;
    int perPage = 10;
    int option = 0;
    int action = ARTICLE_ACTION_EXIT;
    char buf[1024];
    while(1) {
        sb->size = 0;
        sprintf(buf, "%-17s %-17s %-17s\n", "[이름]", "[장르]", "[시청 등급]");
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
            Article *article = (Article*) list->arr[start+i];
            char *articleName = article->name->content;
            if(!checkArticleAgeService(service, user, article)) {
                articleName = "시청 연령 제한 작품";
            }
            char *categoryName = CATEGORY_NAMES[article->category];
            char *ratingName = RATING_NAMES[article->rating];
            sprintf(buf, "[%d] %-17s %-17s %-17s", 
                article->id, articleName, categoryName, ratingName);
            appendMStringBuilder(sb, 2, buf, "\n");
        }
        appendMStringBuilder(sb, 7, SIMPLE_LINE, TOOLTIP_CHOICE, TOOLTIP_PAGE,
            TOOLTIP_CONFIRM, TOOLTIP_QUERY, TOOLTIP_EXIT, SIMPLE_LINE);
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
            Article *article = list->arr[start+option];
            _articleDetailScreen(service, user, article);
            continue;
        }
        if(key == KEYS.lowercaseQ || key == KEYS.uppercaseQ) { // Query
            action = ARTICLE_ACTION_QUERY;
            break;
        }
    }
    freeStringBuilder(sb);
    return action;
}

void userArticleListByCategoryScreen(Service *service, User *user, Category category) {
    char *categoryName = CATEGORY_NAMES[category];
    char buf[1024];
    while(1) {
        sprintf(buf, "%s - %s", "작품 목록", CATEGORY_NAMES[category]);
        ArrayList *list = getArticlesByCategoryService(service, category);
        int action = _articleListScreen(service, user, list, buf);
        freeArrayList(list);
        if(action == ARTICLE_ACTION_EXIT) {
            break;
        }
        while(action == ARTICLE_ACTION_QUERY) {
            String *query = receiveStringDialog("검색어를 입력해주세요. \n초성 검색: 인터스텔라->ㅇㅌㅅㅌㄹ", 30);
            ArrayList *queryResult = queryArticlesService(service, query);
            freeString(query);
            // category filtering
            int k = 0;
            for(int i = 0; i<queryResult->size; i++) {
                Article *article = queryResult->arr[i];
                if(article->category != category) continue;
                queryResult->arr[k++] = article;
            }
            queryResult->size = k;
            sprintf(buf, "작품 검색(%s) - '%s'", categoryName, query->content);
            action = _articleListScreen(service, user, queryResult, buf);
            freeArrayList(queryResult);
        }
    }
}
void userArticleListScreen(Service *service, User *user) {
    char buf[1024];
    while(1) {
        ArrayList *list = getAllArticlesService(service);
        int action = _articleListScreen(service, user, list, buf);
        freeArrayList(list);
        if(action == ARTICLE_ACTION_EXIT) {
            break;
        }
        while(action == ARTICLE_ACTION_QUERY) {
            String *query = receiveStringDialog("검색어를 입력해주세요. \n초성 검색: 인터스텔라->ㅇㅌㅅㅌㄹ", 30);
            ArrayList *queryResult = queryArticlesService(service, query);
            freeString(query);
            sprintf(buf, "작품 검색 - '%s'", query->content);
            action = _articleListScreen(service, user, queryResult, buf);
            freeArrayList(queryResult);
        }
    }
}