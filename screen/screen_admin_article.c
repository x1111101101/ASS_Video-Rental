#include <stdio.h>
#include "../util/texts.h"
#include "../util/uiutil.h"
#include "../util/mystructures.h"
#include "../util/keylistener.h"
#include "../util/math.h"
#include "../util/debug.h"
#include "../service.h"
#include "../db.h"
#include "../video.h"
#include "../article.h"
#include "../user.h"
#include "../rental.h"
#include "screen.h"

#define STOCK_ACTION_EXIT  (0)
#define STOCK_ACTION_QUERY  (1)
#define STOCK_ACTION_REFRESH  (2)

static void createVideoScreen(Service *service, Article *article) {
    String *videoId = receiveStringDialog("입고 비디오의 식별 아이디를 입력해주세요.", 60);
    Response res = createVideoService(service, article, videoId);
    while(!res.succeed) {
        dialog(res.msg->content);
        freeString(videoId);
        freeString(res.msg);
        videoId = receiveStringDialog("입고 비디오의 식별 아이디를 입력해주세요.", 60);
    }
    freeString(res.msg);
    freeString(videoId);
}

static void* _articleEditScreen(Article *article) {
    StringBuilder *sb = newStringBuilder();
    StringBuilder *merge = newStringBuilder();
    int optionSize = 5;
    int option = 0;
    String *keys[] = {
        newString("작품 이름: "),
        newString("작품 설명: "),
        newString("장르: "),
        newString("시청 등급: "),
        newString("[완료]"),
    };
    while(1) {
        sb->size = 0;
        merge->size = 0;
        String *selected = keys[option];
        appendMStringBuilder(merge, 2, "▶ ", selected->content);
        keys[option] = newString(merge->string);
        appendSpaceStringBuilder(sb);
        appendMStringBuilder(sb, 4, SIMPLE_LINE, "작품 관리\n", SIMPLE_LINE, "\n");
        appendMStringBuilder(sb, 3, keys[0]->content, article->name->content, "\n\n");
        appendMStringBuilder(sb, 3, keys[1]->content, article->description->content, "\n\n");
        appendMStringBuilder(sb, 3, keys[2]->content, CATEGORY_NAMES[article->category], "\n\n");
        appendMStringBuilder(sb, 3, keys[3]->content, RATING_NAMES[article->rating], "\n\n");
        appendStringBuilder(sb, keys[4]->content);
        appendStringBuilder(sb, "\n\n");
        appendMStringBuilder(sb, 4, SIMPLE_LINE, TOOLTIP_CHOICE, TOOLTIP_CONFIRM_OR_EDIT, TOOLTIP_EXIT);
        freeString(keys[option]);
        keys[option] = selected;
        printf(sb->string);
        int key = mygetch();
        if(key == KEYS.esc) {
            article = NULL;
            break;
        }
        if(key == KEYS.arrowDown || key == KEYS.arrowUp) {
            int delta = 1 - 2*(key == KEYS.arrowUp);
            option = boundaryMove(option, optionSize, delta);
            continue;
        }
        if(key != KEYS.enter) continue;
        if(option == 0) {
            freeString(article->name);
            article->name = receiveStringDialog("작품 이름을 입력해주세요.", 20);
            continue;
        }
        if(option == 1) {
            freeString(article->description);
            article->description = receiveStringDialog("작품 설명을 입력해주세요.", 200);
            continue;
        }
        if(option == 2) {
            article->category = boundaryMove((int) article->category, CATEGORY_SIZE, 1);
            continue;
        }
        if(option == 3) {
            article->rating = boundaryMove((int) article->rating, RATING_SIZE, 1);
            continue;
        }
        if(option == 4) {
            break;
        }
    }
    freeStringBuilder(sb);
    freeStringBuilder(merge);
    for(int i = 0; i<sizeof(keys)/sizeof(void*); i++) freeString(keys[i]);
    return article;
}

static int stockDetailScreen(Service *service, Video *video) {
    StringBuilder *sb = newStringBuilder();
    int result = 0;
    Rental *rental = NULL;
    User *rentUser;
    if(video->rentalId != -1) {
        rental = getRentalByIdService(service, video->rentalId);
        rentUser = getUserByIdService(service, rental->userId);
    }
    int option = 0;
    int options = 1 + (video->rentalId != -1);
    while(1) {
        sb->size = 0;
        appendSpaceStringBuilder(sb);
        appendMStringBuilder(sb, 5, SIMPLE_LINE, "재고 상세: ", video->videoId->content, "\n", SIMPLE_LINE);
        Date storedDate = video->storedDate;
        String *storedDateStr = toStringDate(&storedDate);
        appendMStringBuilder(sb, 3, "입고: ", storedDateStr->content, "\n");
        freeString(storedDateStr);
        if(rental != NULL) {
            Date rentDate = fromIntDate(rental->rentTime);
            String *rentDateStr = toStringDate(&rentDate);
            appendMStringBuilder(sb, 5, "대여자: ", rentUser->name->content, "(", rentUser->loginId->content, ")\n");
            appendMStringBuilder(sb, 3, "대여일: ", rentDateStr->content, "\n");
            freeString(rentDateStr);
        }
        appendStringBuilder(sb, SIMPLE_LINE);
        appendlnStringBuilder(sb);
        if(option == 0) appendStringBuilder(sb, "▶ ");
        appendStringBuilder(sb, "재고 삭제\n");
        if(options == 2) {
            if(option == 1) appendStringBuilder(sb, "▶ ");
            appendStringBuilder(sb, "반납 처리\n");
        }
        appendlnStringBuilder(sb);
        appendMStringBuilder(sb, 5, SIMPLE_LINE, TOOLTIP_CHOICE, TOOLTIP_CONFIRM, TOOLTIP_EXIT, SIMPLE_LINE);
        printf(sb->string);
        int key = mygetch();
        if(key == KEYS.esc) break;
        if(key == KEYS.arrowDown || key == KEYS.arrowUp) {
            int delta = 1 - 2*(key == KEYS.arrowUp);
            option = boundaryMove(option, options, delta);
            continue;
        }
        if(key != KEYS.enter) continue;
        if(option == 0) {
            if(rental != NULL) {
                if(!boolDialog("현재 대여중인 재고입니다. 삭제할 경우 정상 반납처리됩니다.\n삭제하시겠습니까?")) continue;
            } else if(!boolDialog("삭제하시겠습니까?")) continue;
            result = 1;
            removeVideoService(service, video);
            break;
        }
        if(!boolDialog("반납처리를 진행하시겠습니까?")) continue;
        returnVideoService(service, rental);
        result = 1;
        dialog("반납 처리가 완료되었습니다.");
        break;
    }
    freeStringBuilder(sb);
    return result;
}

static int _manageStockScreen(Service *service, Article *article, ArrayList *list, char *title) {
    StringBuilder *sb = newStringBuilder(sb);
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
        appendMStringBuilder(sb, 7, TOOLTIP_CHOICE, TOOLTIP_PAGE, TOOLTIP_CONFIRM, TOOLTIP_CREATE, TOOLTIP_QUERY, TOOLTIP_EXIT, SIMPLE_LINE);
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
            int result = stockDetailScreen(service, video);
            if(result) {
                action = STOCK_ACTION_REFRESH;
                break;
            }
            continue;
        }
        if(key == KEYS.lowercaseQ || key == KEYS.uppercaseQ) { // Query
            action = STOCK_ACTION_QUERY;
            break;
        }
        if(key == KEYS.lowercaseC || key == KEYS.uppercaseC) {
            createVideoScreen(service, article);
            action = STOCK_ACTION_REFRESH;
            break;
        }
    }
    freeStringBuilder(sb);
    return action;
}

static void manageStock(Service *service, Article *article) {
    ArrayList *list = getVideosService(service, article->id);
    while(1) {
        sortArrayList(list, VIDEO_ID_COMPARATOR);
        int result = _manageStockScreen(service, article, list, "비디오 재고 목록");
        if(result == STOCK_ACTION_EXIT) {
            break;
        }
        if(result == STOCK_ACTION_REFRESH) {
            freeArrayList(list);
            list = getVideosService(service, article->id);
            continue;
        }
        while(result == STOCK_ACTION_QUERY) {
            String *query = receiveStringDialog("재고 식별번호 검색어를 입력해주세요.", 60);
            ArrayList *queryResult = queryVideosService(service, query);
            ArrayList *filtered = newArrayList(queryResult->size);
            for(int i = 0; i<queryResult->size; i++) {
                Video *video = (Video*) queryResult->arr[i];
                if(video->articleId != article->id) continue;
                pushArrayList(filtered, video);
            }
            freeArrayList(queryResult);
            StringBuilder *sb = newStringBuilder();
            appendMStringBuilder(sb, 3, "재고 검색: '", query->content, "'");
            freeString(query);
            result = _manageStockScreen(service, article, filtered, sb->string);
            freeStringBuilder(sb);
            freeArrayList(filtered);
        }
        freeArrayList(list);
        list = getVideosService(service, article->id);
    }
    freeArrayList(list);
}

static Article* editArticle(Service *service, Article *article) {
    Article *clone = cloneArticle(article);
    while(1) {
        void *result = _articleEditScreen(clone);
        if(result == NULL) {
            freeArticle(clone);
            return NULL;
        }
        Response res = editArticleService(service, clone->id, clone);
        if(!res.succeed) {
            dialog(res.msg->content);
            freeString(res.msg);
            continue;
        }
        freeString(res.msg);
        dialog("편집 내용을 저장하였습니다.");
        return clone;
    }
}

// 수정, 삭제 여부 리턴
static int articleDetailScreen(Service *service, Article *article) {
    int result = 0;
    StringBuilder *sb = newStringBuilder();
    char *articleKeys[] = {
        "작품 이름",
        "작품 설명",
        "작품 장르",
        "시청 등급"
    };
    char *options[] = {
        "작품 정보 편집",
        "재고 관리",
        "작품 삭제",
        "대여 기록"
    };
    int option = 0;
    int optionSize = 4;
    while(1) {
        sb->size = 0;
        appendSpaceStringBuilder(sb);
        appendMStringBuilder(sb, 5, SIMPLE_LINE, "작품 관리: ", article->name->content, "\n", SIMPLE_LINE);
        char *articleValues[] = {article->name->content, article->description->content, CATEGORY_NAMES[article->category], RATING_NAMES[article->rating]};
        for(int i = 0; i<4; i++) appendMStringBuilder(sb, 4, articleKeys[i], ": ", articleValues[i], "\n");
        appendMStringBuilder(sb, 1, SIMPLE_LINE);
        for(int i = 0; i<optionSize; i++) {
            if(option == i) appendStringBuilder(sb, "▶ ");
            appendMStringBuilder(sb, 2, options[i], "\n");
        }
        appendStringBuilder(sb, SIMPLE_LINE);
        appendMStringBuilder(sb, 3, TOOLTIP_CHOICE, TOOLTIP_CONFIRM, TOOLTIP_EXIT);
        appendStringBuilder(sb, SIMPLE_LINE);
        printf(sb->string);
        int key = mygetch();
        if(key == KEYS.esc) break;
        if(key == KEYS.arrowDown || key == KEYS.arrowUp) {
            int delta = 1 - 2*(key == KEYS.arrowUp);
            option = boundaryMove(option, optionSize, delta);
            continue;
        }
        if(key != KEYS.enter) continue;
        if(option == 0) { // 작품 정보 편집
            Article *edited = editArticle(service, article);
            if(!edited) continue; 
            result = 1;
            article = edited; 
        } else if(option == 1) { // 재고 관리
            manageStock(service, article);
        } else if(option == 2) { // 작품 삭제
            ArrayList *videos = getVideosService(service, article->id);
            int rent = 0;
            for(int i = 0; i<videos->size; i++) rent += ((Video*) videos->arr[i])->rentalId != -1;
            freeArrayList(videos);
            if(rent > 0) {
                int res = boolDialog("현재 대여중인 재고가 있습니다.\n 만약 삭제할 경우, 정상 반납 처리됩니다. 삭제하시겠습니까?");
                if(!res) continue;
            }
            int res = boolDialog("삭제할 경우 이 작품에 대한 재고, 대여 기록이 모두 삭제됩니다. 정말 삭제하시겠습니까?");
            if(!res) continue;
            removeArticleService(service, article);
            dialog("정상적으로 삭제되었습니다.");
            result = 1;
            break;
        } else if(option == 3) { // 대여 기록

        }
    }
    freeStringBuilder(sb);
    return result;
}

static void createArticleScreen(Service *service) {
    Article *article = newArticle();
    while(1) {
        void *result = _articleEditScreen(article);
        if(result == NULL) {
            freeArticle(article);
            return;
        }
        Response response = createArticleService(service, article);
        if(response.succeed) {
            freeString(response.msg);
            dialog("작품을 성공적으로 생성하였습니다!");
            return;
        }
        dialog(response.msg->content);
        freeString(response.msg);
        continue;
    }   
}

#define ARTICLE_MANAGE_ACTION_EXIT (0)
#define ARTICLE_MANAGE_ACTION_QUERY (1)
#define ARTICLE_MANAGE_ACTION_REFRESH (2)

int _articleManageScreen(Service *service, ArrayList *list) {
    int action = ARTICLE_MANAGE_ACTION_EXIT;
    StringBuilder *sb = newStringBuilder(sb);
    static int pageRem = 0;
    int page = pageRem;
    int perPage = 10;
    int option = 0;
    while(1) {
        sb->size = 0;
        appendSpaceStringBuilder(sb);
        appendMStringBuilder(sb, 3, SIMPLE_LINE, "작품 관리\n", SIMPLE_LINE);
        int pageSize = (list->size + perPage-1) / perPage;
        int start = page*perPage;
        int end = MIN(list->size, start + perPage);
        int size = end-start;
        option = fitRange(option, 0, size-1);
        if(size == 0) appendStringBuilder(sb, "페이지가 비어있습니다.\n");
        else for(int i = 0; i<size; i++) {
            if(i == option) appendStringBuilder(sb, "▶ ");
            Article *article = (Article*) list->arr[start+i];
            static char buf[1024];
            sprintf(buf, "[%d] %-20s", article->id, article->name->content);
            appendMStringBuilder(sb, 2, buf, "\n");
        }
        appendStringBuilder(sb, SIMPLE_LINE);
        appendMStringBuilder(sb, 7, TOOLTIP_CHOICE, TOOLTIP_PAGE, TOOLTIP_CONFIRM, TOOLTIP_QUERY, TOOLTIP_CREATE, TOOLTIP_EXIT, SIMPLE_LINE);
        printf(sb->string);
        int key = mygetch();
        if(key == KEYS.esc) {
            action = ARTICLE_MANAGE_ACTION_EXIT;
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
            Article *article = list->arr[start+option];
            if(articleDetailScreen(service, article)) {
                freeArrayList(list);
                list = getAllArticlesService(service);
            }
            continue;
        }
        if(key == KEYS.lowercaseC || key == KEYS.uppercaseC) { // Create
            createArticleScreen(service);
            action = ARTICLE_MANAGE_ACTION_REFRESH;
            break;
        }
        if(key == KEYS.lowercaseQ || key == KEYS.uppercaseQ) { // Query
            action = ARTICLE_MANAGE_ACTION_QUERY;
            break;
        }
    }
    freeStringBuilder(sb);
    if(action == ARTICLE_MANAGE_ACTION_EXIT) pageRem = 0;
    else pageRem = page;
    return action;
}

void articleManageScreen(Service *service) {
    ArrayList *list = getAllArticlesService(service);
    while(1) {
        sortArrayList(list, VIDEO_ID_COMPARATOR);
        int result = _articleManageScreen(service, list);
        if(result == ARTICLE_MANAGE_ACTION_EXIT) {
            break;
        }
        if(result == ARTICLE_MANAGE_ACTION_REFRESH) {
            freeArrayList(list);
            list = getAllArticlesService(service);
            continue;
        }
        while(result == STOCK_ACTION_QUERY) {
            String *query = receiveStringDialog("검색어를 입력해주세요.", 60);
            ArrayList *queryResult = queryArticlesService(service, query);
            freeString(query);
            result = _articleManageScreen(service, queryResult);
            freeArrayList(queryResult);
        }
        freeArrayList(list);
        list = getAllArticlesService(service);
    }
    freeArrayList(list);
}