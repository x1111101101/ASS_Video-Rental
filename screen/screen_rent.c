#include <stdio.h>
#include "../util/texts.h"
#include "../util/uiutil.h"
#include "../util/debug.h"
#include "../util/mystructures.h"
#include "../util/keylistener.h"
#include "../util/math.h"
#include "../service.h"
#include "../db.h"
#include "../video.h"
#include "../article.h"
#include "../user.h"
#include "../rental.h"
#include "screen.h"

static int _rentalListScreen(Service *service, User *user, ArrayList* list, char *title) {
    StringBuilder *sb = newStringBuilder(sb);
    int page = 0;
    int perPage = 10;
    int option = 0;
    int action = RENTAL_ACTION_EXIT;
    char buf[1024];
    while(1) {
        sb->size = 0;
        sprintf(buf, "%-17s %-17s %-30s %-30s\n", "   [작품]", "[비디오 식별번호]","[대여일]", "[반납일]");
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
            Rental *rental = (Rental*) list->arr[start+i];
            Video *video = (Video*) getVideoByIdService(service, rental->videoId);
            Article *article = (Article*) getArticleByIdService(service, video->articleId);
            Date rentDate = fromIntDate(rental->rentTime);
            String *rentDateStr = toStringDate(&rentDate);
            char *articleName = article->name->content;
            char *videoIdName = video->videoId->content;
            if(rental->returnTime == -1) {
                sprintf(buf, "[%d] %-17s %-17s %-30s 대여중", article->id, articleName, videoIdName, rentDateStr->content);
            } else {
                Date returnDate = fromIntDate(rental->returnTime);
                String *returnDateStr = toStringDate(&returnDate);
                sprintf(buf, "[%d]%-17s %-17s %-30s %-30s", article->id, articleName, videoIdName, rentDateStr->content, returnDateStr->content);
                freeString(returnDateStr);
            }
            freeString(rentDateStr);
            appendMStringBuilder(sb, 2, buf, "\n");
        }
        appendStringBuilder(sb, SIMPLE_LINE);
        appendMStringBuilder(sb, 5, TOOLTIP_CHOICE, TOOLTIP_PAGE, TOOLTIP_CONFIRM, TOOLTIP_EXIT, SIMPLE_LINE);
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
            
            continue;
        }
    }
    freeStringBuilder(sb);
    return action;
}

int rentalAdminScreen(Service *service, ArrayList *list) {
    StringBuilder *sb = newStringBuilder(sb);
    int page = 0;
    int perPage = 10;
    int option = 0;
    int action = RENTAL_ACTION_EXIT;
    char buf[1024];
    while(1) {
        sb->size = 0;
        sprintf(buf, "%-17s %-17s %-17s %-17s\n", "   [작품]", "[비디오 식별번호]","[대여일]", "[반납일]");
        appendSpaceStringBuilder(sb);
        appendMStringBuilder(sb, 5, SIMPLE_LINE, "대여 목록", "\n\n", buf, SIMPLE_LINE);
        int pageSize = (list->size + perPage-1) / perPage;
        page = fitRange(page, 0, pageSize-1);
        int start = page*perPage;
        int end = MIN(list->size, start + perPage);
        int size = end-start;
        option = fitRange(option, 0, size-1);
        if(size == 0) appendStringBuilder(sb, "페이지가 비어있습니다.\n");
        else for(int i = 0; i<size; i++) {
            if(option == i) appendStringBuilder(sb, "▶ ");
            Rental *rental = (Rental*) list->arr[start+i];
            Video *video = (Video*) getVideoByIdService(service, rental->videoId);
            Article *article = (Article*) getArticleByIdService(service, video->articleId);
            Date rentDate = fromIntDate(rental->rentTime);
            String *rentDateStr = toStringDate(&rentDate);
            if(rental->returnTime == -1) {
                sprintf(buf, "[%d]%-17s %-17s %-17s 대여중", article->id, article->name->content, video->videoId->content, rentDateStr->content);
            } else {
                Date returnDate = fromIntDate(rental->returnTime);
                String *returnDateStr = toStringDate(&returnDate);
                sprintf(buf, "[%d]%-17s %-17s %-17s %-17s", article->id, article->name->content, video->videoId->content, rentDateStr->content, returnDateStr->content);
                freeString(returnDateStr);
            }
            freeString(rentDateStr);
            appendMStringBuilder(sb, 2, buf, "\n");
        }
        appendStringBuilder(sb, SIMPLE_LINE);
        appendMStringBuilder(sb, 5, TOOLTIP_CHOICE, TOOLTIP_PAGE, TOOLTIP_CONFIRM, TOOLTIP_EXIT, SIMPLE_LINE);
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
            Rental *rental = getArrayList(list, option);
            if(rental->returnTime != -1) {
                dialog("반납이 완료된 대여 기록입니다.");
                continue;
            }
            if(!boolDialog("해당 대여를 반납처리하시겠습니까?")) continue;
            returnVideoService(service, rental);
            dialog("정상적으로 반납처리되었습니다.");
            continue;
        }
        if(key == KEYS.lowercaseQ || key == KEYS.uppercaseQ) { // Query
            action = RENTAL_ACTION_QUERY;
            break;
        }
    }
    freeStringBuilder(sb);
    return action;
}

void rentalListScreen(Service *service, User *user) {
    int result = 0;
    ArrayList *list = getRentalsByUserService(service, user->id);
    while(1) {
        sortArrayList(list, RENTAL_ID_COMPARATOR);
        result = _rentalListScreen(service, user, list, "대여 기록");
        freeArrayList(list);
        if(result == RENTAL_ACTION_EXIT) break;
        if(result == RENTAL_ACTION_REFRESH) {
            freeArrayList(list);
            list = getRentalsByUserService(service, user->id);
        }
    }
    freeArrayList(list);
}

static void _filterRenting(ArrayList *list) {
    int k = 0;
    for(int i = 0; i<list->size; i++) {
        Rental *rental = (Rental*) getArrayList(list, i);
        if(rental->returnTime == -1) list->arr[k++] = rental;
    }
    list->size = k;
}

void returnScreen(Service *service, User *user) {
    StringBuilder *sb = newStringBuilder();
    char buf[1024];
    int option = 0;
    while(1) {
        sb->size = 0;
        appendSpaceStringBuilder(sb);
        appendMStringBuilder(sb, 3, SIMPLE_LINE, "반납하기\n", SIMPLE_LINE);
        sprintf(buf, "%-17s %-17s %-30s\n", "   [작품]", "[비디오 식별번호]","[대여일]");
        appendStringBuilder(sb, buf);
        ArrayList *list = getRentalsByUserService(service, user->id);
        _filterRenting(list);
        int options = list->size;
        fitRange(option, 0, options-1);
        if(options == 0) appendStringBuilder(sb, "대여중인 비디오가 없습니다.\n");
        else for(int i = 0; i<list->size; i++) {
            if(option == i) appendStringBuilder(sb, "▶ ");
            Rental *rental = (Rental*) getArrayList(list, i);
            Video *video = getVideoByIdService(service, rental->videoId);
            Article *article = getArticleByIdService(service, video->articleId);
            char *articleName = article->name->content;
            char *videoIdName = video->videoId->content;
            Date rentDate = fromIntDate(rental->rentTime);
            String *rentDateStr = toStringDate(&rentDate);
            sprintf(buf, "[%d] %-17s %-17s %-30s 대여중\n", article->id, articleName, videoIdName, rentDateStr->content);
            freeString(rentDateStr);
            appendStringBuilder(sb, buf);
        }
        appendMStringBuilder(sb, 5, SIMPLE_LINE, TOOLTIP_CHOICE, TOOLTIP_CONFIRM, TOOLTIP_EXIT, SIMPLE_LINE);
        printf(sb->string);
        int key = mygetch();
        if(key == KEYS.esc) {
            freeArrayList(list);
            break;
        }
        if(key == KEYS.arrowDown || key == KEYS.arrowUp) { // Choice
            int delta = 1 - 2*(key == KEYS.arrowUp);
            option = boundaryMove(option, options, delta);
            freeArrayList(list);
            continue;
        }
        if(key != KEYS.enter) {
            freeArrayList(list);
            continue;
        }
        if(options == 0) continue;
        Rental *rental = (Rental*) getArrayList(list, option);
        freeArrayList(list);
        Video *video = getVideoByIdService(service, rental->videoId);
        Article *article = getArticleByIdService(service, video->articleId);
        sprintf(buf, "%s 작품의 비디오를 반납하시겠습니까?", article->name->content);
        if(!boolDialog(buf)) continue;
        returnVideoService(service, rental);
        dialog("비디오를 정상적으로 반납하였습니다.");
    }
    freeStringBuilder(sb);
}