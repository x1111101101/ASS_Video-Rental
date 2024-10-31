#include <stdlib.h>
#include <stdio.h>
#include "util/date.h"
#include "util/debug.h"
#include "util/uiutil.h"
#include "util/texts.h"
#include "service.h"
#include "db.h"
#include "user.h"
#include "article.h"
#include "rental.h"
#include "video.h"

static int _isAdmin(Service *service, int userId);
static int _isAdminLoginId(String *loginId);
static int _validateLoginId(String *loginId, Response *target);
static int _validateUserName(String *name, Response *target);
static int _validatePassword(String *password, Response *target);
static int _validatePhoneNumber(String *phone, Response *target);
static int _validateBirthday(Date birth, Response *target);

static void throw(char *msg) {
    dialog(msg);
    exit(1);
}

Service* newService() {
    Service *s = malloc(sizeof(Service));
    s->loginUser = NULL;
    s->users = newUserDB();
    s->rentals = newRentalDB();
    s->articles = newArticleDB();
    s->videos = newVideoDB();
    return s;
}

Service* loadService() {
    FILE *file = fopen("save.txt", "rb");
    if(file == NULL) {
        d("save file missing\n");
        return newService();
    }
    Service *service = malloc(sizeof(Service));
    service->loginUser = NULL;
    service->users = USER_DB_SERIALIZER.deserialize(file);
    service->articles = ARTICLE_DB_SERIALIZER.deserialize(file);
    service->rentals = RENTAL_DB_SERIALIZER.deserialize(file);
    service->videos = VIDEO_DB_SERIALIZER.deserialize(file);
    return service;
}

static User* findUserByLoginId(Service *service, String *loginId) {
    return (User*) findUniqueCacheDB(service->users, USER_CACHE_LOGINID, loginId);
}

static User* createAdminAccount() {
    d("createAdminAccount");
    User *user = newUser();
    String *initialPw = newString("admin0000");
    freeString(user->loginId);
    freeString(user->phone);
    freeString(user->name);
    user->loginId = newString("admin");
    user->phone = newString("0000");
    user->passwordHash = initialPw->hashcode;
    user->birthday = (Date) {2003, 10, 5};
    user->name = newString("관리자");
    freeString(initialPw);
    return user;
}

static void saveService(Service *service) {
    d("SAVING SERVICE");
    FILE *file = fopen("save.txt", "wb");
    if(file == NULL) {
        dialog("파일 저장에 실패하였습니다.");
        exit(1);
    }
    USER_DB_SERIALIZER.serialize(file, service->users);
    ARTICLE_DB_SERIALIZER.serialize(file, service->articles);
    RENTAL_DB_SERIALIZER.serialize(file, service->rentals);
    VIDEO_DB_SERIALIZER.serialize(file, service->videos);
    fclose(file);
}

void initService(Service *service) {
    String *adminId = newString("admin");
    if(findUserByLoginId(service, adminId) == NULL) {
        insertDB(service->users, createAdminAccount());
        saveService(service);
    }
    freeString(adminId);
}

// Rent, Return
void returnVideoService(Service *service, Rental *rental) {
    Video *video = findByIdDB(service->videos, rental->videoId);
    if(video->rentalId != rental->id) throw("Illegal State");
    if(rental->returnTime != -1) throw("Illegal Argument");
    Date today = currentDate();
    rental->returnTime = toIntDate(&today);
    video->rentalId = -1;
    saveService(service);
}

Response rentVideoService(Service *service, int userId, int videoId) {
    Video *video = getVideoByIdService(service, videoId);
    User *user = getUserByIdService(service, userId);
    if(!video || !user) throw("Illegal Argument");
    Article *article = getArticleByIdService(service, video->articleId);
    if(!checkArticleAgeService(service, user, article)) { // 시청 가능한 연령인지 확인
        return (Response) {0, newString("이 작품을 시청할 수 없는 연령의 사용자입니다.")};
    }
    if(video->rentalId != -1) { // 대여중인 비디오인지 확인
        return (Response) {0, newString("이미 대여중인 비디오입니다.")};
    }
    int rentingCount = 0;
    ArrayList *rentals = getRentalsByUserService(service, userId);
    for(int i = 0; i<rentals->size; i++) {
        Rental *rental = (Rental*) getArrayList(rentals, i);
        if(rental->returnTime != -1) continue;
        Video *rentVideo = getVideoByIdService(service, rental->videoId);
        Article *rentArticle = getArticleByIdService(service, video->articleId);
        if(article == rentArticle) {
            freeArrayList(rentals);
            return (Response) {0, newString("이미 같은 작품을 대여중입니다.")};
        }
        rentingCount++;
    }
    if(rentingCount >= 10) { // 동시 대여 가능 최대 수량 초과
        return (Response) {0, newString("동시에 최대 10개의 비디오만 대여할 수 있습니다.\n기존 대여 비디오를 반납 후 시도해주세요.")};
    }
    freeArrayList(rentals);
    Rental *rental = newRental();
    rental->userId = userId;
    rental->videoId = videoId;
    Date today = currentDate();
    rental->rentTime = toIntDate(&today);
    insertDB(service->rentals, rental);
    video->rentalId = rental->id;
    saveService(service);
    return (Response) {1, newString("")};
}

static Response _editUserService(Service *service, int id, User *edited) {
    d("_editUser");
    Response response;
    if(edited->id != id) throw("Illegal Argument");
    // check name
    if(!_validateUserName(edited->name, &response)) return response;
    // check login id
    if(_isAdmin(service, id) && !_isAdminLoginId(edited->loginId)) {
        return (Response) {0, newString("관리자 게정 아이디는 변경이 불가능합니다.")};
    }
    if(!_validateLoginId(edited->loginId, &response)) return response;
    User *loginIdDuplicated = findUniqueCacheDB(service->users, USER_CACHE_LOGINID, edited->loginId);
    if(loginIdDuplicated != NULL && loginIdDuplicated->id != id) return (Response) {0, newString("해당 아이디의 사용자가 이미 존재합니다.")};
    // check phone number
    if(!_validatePhoneNumber(edited->phone, &response)) return response;
    User *phoneDuplicated = findUniqueCacheDB(service->users, USER_CACHE_PHONE, edited->phone);
    if(phoneDuplicated != NULL && phoneDuplicated->id != id) {
        return (Response) {0, newString("해당 연락처로 등록된 사용자가 이미 존재합니다.")};
    }
    // check birthday
    if(!_validateBirthday(edited->birthday, &response)) return response;
    // valid form
    deleteDB(service->users, id);
    insertDB(service->users, edited);
    saveService(service);
    return (Response) {1, newString("")};
}

// Accounts
Response editUserService(Service *service, int id, SignupForm form) {
    d("editUser");
    int passwordLen = countUtf8Char(form.password);
    if(passwordLen > 0 && passwordLen < 8) return (Response) {0, newString("비밀번호는 8자 이상으로 입력해야합니다.")};
    User *edited = malloc(sizeof(User));
    User *original = findByIdDB(service->users, id);
    edited->birthday = form.birthday;
    edited->name = cloneString(form.name);
    edited->id = id;
    edited->phone = cloneString(form.phone);
    edited->loginId = cloneString(form.loginId);
    if(passwordLen == 0) {
        edited->passwordHash = original->passwordHash;
    } else {
        edited->passwordHash = form.password->hashcode;
    }
    Response res = _editUserService(service, id, edited);
    df1("edited: %s\n", edited->name->content);
    if(res.succeed) return res;
    freeUser(edited);
    return res;
}

Response signupService(Service *service, SignupForm form) {
    Response response;
    // check name
    if(!_validateUserName(form.name, &response)) return response;
    // check login id
    if(!_validateLoginId(form.loginId, &response)) return response;
    void *loginIdDuplicated = findUniqueCacheDB(service->users, USER_CACHE_LOGINID, form.loginId);
    if(loginIdDuplicated != NULL) return (Response) {0, newString("해당 아이디의 사용자가 이미 존재합니다.")};
    // check password
    if(!_validatePassword(form.password, &response)) return response;
    // check phone number
    if(!_validatePhoneNumber(form.phone, &response)) return response;
    void *phoneDuplicated = findUniqueCacheDB(service->users, USER_CACHE_PHONE, form.phone);
    if(phoneDuplicated != NULL) {
        return (Response) {0, newString("해당 연락처로 등록된 사용자가 이미 존재합니다.")};
    }
    // check birthday
    if(!_validateBirthday(form.birthday, &response)) return response;
    // valid form
    int passwordHash = form.password->hashcode;
    User *user = newUser();
    freeString(user->loginId);
    freeString(user->phone);
    freeString(user->name);
    user->loginId = newString(form.loginId->content);
    user->name = newString(form.name->content);
    user->phone = newString(form.phone->content);
    user->birthday = form.birthday;
    user->passwordHash = form.password->hashcode;
    insertDB(service->users, user);
    saveService(service);
    return (Response) {1, newString("")};
}

int tryLoginService(Service *service, String *loginId, String *password) {
    User *first = findUniqueCacheDB(service->users, USER_CACHE_LOGINID, loginId);
    if(first == NULL) return -1;
    int hashcode = password->hashcode;
    if(first->passwordHash != hashcode) {
        return -2;
    }
    service->loginUser = first;
    return first->id;
}

int isLoginedService(Service *service) {
    return service->loginUser != NULL;
}

User* getLoginService(Service *service) {
    return service->loginUser;
}

void logoutService(Service *service) {
    service->loginUser = NULL;
}
ArrayList* getAllUsersService(Service *service) {
    return findAllDB(service->users);
}
ArrayList* queryUserService(Service *service, String *query) {
    ArrayList *byName = queryDB(service->users, USER_SEARCH_NAME, query);
    ArrayList *byPhone = queryDB(service->users, USER_SEARCH_PHONE, query);
    int cap = byName->size + byPhone->size;
    HashMap *map = newHashMap(cap+1, HASH_PTR, EQUALS_PTR);
    for(int i = 0; i<byName->size; i++) {
        NGramCache *cache = byName->arr[i];
        putHashMap(map, cache->instance, cache);
    }
    for(int i =0; i<byPhone->size; i++) {
        NGramCache *cache = byPhone->arr[i];
        NGramCache *prev = getHashMap(map, cache->instance);
        if(prev == NULL) {
            putHashMap(map, cache->instance, cache);
            continue;
        }
        if(prev->weight < cache->weight) {
            removeHashMap(map, cache->instance);
            free(prev);
            putHashMap(map, cache->instance, cache);
            continue;
        }
        free(cache);
    }
    ArrayList *sorted = newArrayList(map->size);
    HashMapIterator it = newHashMapIterator(map);
    while(hasNextEntry(&it)) {
        Entry e = nextEntry(&it);
        pushArrayList(sorted, e.value);
    }
    freeHashMap(map);
    qsort(sorted->arr, sorted->size, sizeof(void*), compareNGramCaches);
    for(int i = 0; i<sorted->size; i++) {
        NGramCache *cache = sorted->arr[i];
        sorted->arr[i] = cache->instance;
        free(cache);
    }
    return sorted;
}
User* getUserByIdService(Service *service, int id) {
    return findByIdDB(service->users, id);
}
Response removeUserService(Service *service, int id) {
    if(_isAdmin(service, id))
        return (Response) {0, newString("관리자 계정은 삭제가 불가능합니다.")};
    ArrayList* rentals = getRentalsByUserService(service, id);
    for(int i = 0; i<rentals->size; i++) removeRentalService(service, (Rental*) rentals->arr[i]);
    freeArrayList(rentals);
    deleteDB(service->users, id);
    saveService(service);
    return (Response) {1, newString("")};
}
void changeUserPasswordService(Service *service, User *user, String *password) {
    if(countUtf8Char(password) < 8) throw("Illegal Argument");
    user->passwordHash = password->hashcode;
    saveService(service);
}

// Article CRUDQ
Article* getArticleByIdService(Service *service, int id) {
    return findByIdDB(service->articles, id);
}
ArrayList* getArticlesByCategoryService(Service *service, Category category) {
    return findCacheDB(service->articles, ARTICLE_CACHE_CATEGORY, (void*) category);
}
Response createArticleService(Service *service, Article *article) {
    if(countUtf8Char(article->name) <= 0) return (Response) {0, newString("작품의 이름이 비어있습니다.")};
    article->id = -1;
    insertDB(service->articles, article);
    saveService(service);
    return (Response) {1, newString("")};
}
Response editArticleService(Service *service, int id, Article *edited) {
    if(countUtf8Char(edited->name) <= 0) return (Response) {0, newString("작품의 이름이 비어있습니다.")};
    Article *nameDuplicated = findUniqueCacheDB(service->articles, ARTICLE_CACHE_NAME, edited->name);
    if(nameDuplicated != NULL && nameDuplicated->id != id) return (Response) {0, newString("해당 이름의 작품이 이미 등록되어있습니다.")};
    if(id != edited->id) throw("Illegal Argument");
    deleteDB(service->articles, id);
    insertDB(service->articles, edited);
    saveService(service);
    return (Response) {1, newString("")};
}
ArrayList* queryArticlesService(Service *service, String *query) {
    ArrayList* list = queryDB(service->articles, ARTICLE_CACHE_NAME, query);
    for(int i = 0; i<list->size; i++) {
        NGramCache *cache = getArrayList(list, i);
        list->arr[i] = cache->instance;
        free(cache);
    }
    return list;
}
ArrayList* getAllArticlesService(Service *service) {
    return findAllDB(service->articles);
}
void removeArticleService(Service *service, Article *article) {
    ArrayList *videos = getVideosService(service, article->id);
    for(int i = 0; i<videos->size; i++) removeVideoService(service, (Video*) videos->arr[i]);
    freeArrayList(videos);
    deleteDB(service->articles, article->id);
    saveService(service);
}
int checkArticleAgeService(Service *service, User *user, Article *article) {
    Date current = currentDate();
    Date birth = user->birthday;
    int userAge = current.year - birth.year + 1;
    df1("usrage: %d\n", userAge);
    int ageTable[] = {0, 12, 15, 19};
    return userAge >= ageTable[article->rating];
}
VideoStock getVideoStockService(Service *service, int articleId) {
    ArrayList *videos = getVideosService(service, articleId);
    int left = 0;
    for(int i = 0; i<videos->size; i++) {
        Video *video = (Video*) getArrayList(videos, i);
        left += video->rentalId == -1;
    }
    freeArrayList(videos);
    return (VideoStock) { videos->size, left };
}
Rental* getCurrentRentalByArticleAndUser(Service *service, int articleId, int userId) {
    Rental *result = NULL;
    ArrayList *videos = getVideosService(service, articleId);
    for(int i = 0; i<videos->size; i++) {
        Video *video = getArrayList(videos, i);
        if(video->rentalId == -1) continue;
        Rental *rental = getRentalByIdService(service, video->rentalId);
        if(rental->userId != userId) continue;
        result = rental;
        break;
    }
    freeArrayList(videos);
    return result;
}

int isArticleRentedByUserService(Service *service, int articleId, int userId) {
    return getCurrentRentalByArticleAndUser(service, articleId, userId) != NULL;
}

// Video CRUDQ
Response createVideoService(Service *service, Article *article, String *videoId) {
    int len = countUtf8Char(videoId);
    if(len < 2 || len > 60) return (Response) {0, newString("비디오 식별 번호는 2~60자의 문자열이여야합니다.")};
    void *duplicated = getVideoByVideoIdService(service, videoId);
    if(duplicated != NULL) return (Response) {0, newString("다른 비디오 재고와 식별 번호가 중복됩니다.")};
    Article *articleInst = getArticleByIdService(service, article->id);
    if(articleInst == NULL) throw("Illegal Argument: invalid article");
    Video *video = newVideo();
    freeString(video->videoId);
    video->videoId = newString(videoId->content);
    video->articleId = article->id;
    insertDB(service->videos, video);
    saveService(service);
    return (Response) {1, newString("")};
}
Video* getVideoByVideoIdService(Service *service, String *videoId) {
    return findUniqueCacheDB(service->videos, VIDEO_CACHE_VIDEOID, videoId);
}
Video* getVideoByIdService(Service *service, int id) {
    return findByIdDB(service->videos, id);
}
ArrayList* getVideosService(Service *service, int articleId) {
    return findCacheDB(service->videos, VIDEO_CACHE_ARTICLEID, (void*) articleId);
}
ArrayList* queryVideosService(Service *service, String *query) {
    ArrayList *result = queryDB(service->videos, VIDEO_SEARCH_VIDEOID, query);
    for(int i = 0; i<result->size; i++) result->arr[i] = ((NGramCache*) result->arr[i])->instance;
    return result;
}
void removeVideoService(Service *service, Video *video) {
    d("removeVideoService");
    ArrayList *rentals = getRentalsByVideoService(service, video->id);
    for(int i = 0; i<rentals->size; i++) removeRentalService(service, (Rental*) rentals->arr[i]);
    freeArrayList(rentals);
    deleteDB(service->videos, video->id);
    saveService(service);
}

// Rental CRUDQ
ArrayList* getRentalsByArticleService(Service *service, int articleId) {
    ArrayList *videos = getVideosService(service, articleId);
    ArrayList *rentals = newArrayList(videos->size * 2);
    for(int i = 0; i<videos->size; i++) {
        ArrayList *list = getRentalsByVideoService(service, ((Video*) videos->arr[i])->id);
        for(int k = 0; k<list->size; k++) pushArrayList(rentals, list->arr[k]);
        freeArrayList(list);
    }
    freeArrayList(videos);
    return rentals;
}
ArrayList* getRentalsByVideoService(Service *service, int videoId) {
    return findCacheDB(service->rentals, RENTAL_CACHE_VIDEOID, (void*) videoId);
}
ArrayList* getRentalsByUserService(Service *service, int userId) {
    return findCacheDB(service->rentals, RENTAL_CACHE_USERID, (void*) userId);
}
void removeRentalService(Service *service, Rental *rental) {
    d("removeRentalService");
    Video *video = findByIdDB(service->videos, rental->videoId);
    if(video == NULL) throw("Illegal State");
    if(video->rentalId == rental->id) video->rentalId = -1;
    deleteDB(service->rentals, rental->id);
    saveService(service);
}
Rental* getRentalByIdService(Service *service, int id) {
    return (Rental*) findByIdDB(service->rentals, id);
}
ArrayList* getAllRentalsService(Service *service) {
    return findAllDB(service->rentals);
}

static int _isAdminLoginId(String *loginId) {
    String *adminId = newString("admin");
    int result = EQUALS_STRING(adminId, loginId);
    freeString(adminId);
    return result;
}

static int _isAdmin(Service *service, int userId) {
    User *user = findByIdDB(service->users, userId);
    int result = user != NULL && _isAdminLoginId(user->loginId);
    return result;
}

/**
 * User Validations
 */
static int _validatePhoneNumber(String *phone, Response *target) {
    d("_validatePhoneNumber");
    CharIterator it = newCharIterator(phone->content);
    while(hasNextChar(&it)) {
        int next = nextChar(&it);
        if(next < '0' || next > '9') {
            *target = (Response) {0, newString("연락처는 숫자만 입력해주세요.")};
            return 0;
        }
    }
    return 1;
}
static int _validateBirthday(Date birth, Response *target) {
    d("_validateBirthday");
    Date now = currentDate(); 
    if(!isValidDate(&birth)) {
        *target = (Response) {0, newString("잘못된 생년월일을 입력하셨습니다.")};
        return 0;
    }
    if(isPastDate(&now, &birth)) {
        StringBuilder *sb = newStringBuilder();
        String *nowStr = toStringDate(&now);
        appendMStringBuilder(sb, 3, "생년월일은 ", nowStr->content, " 이전 날짜만 입력 가능합니다.");
        String *msg = newString(sb->string);
        freeString(nowStr);
        freeStringBuilder(sb);
        *target = (Response) {0, msg};
        return 0;
    }
    return 1;
}

static int _validateUserName(String *name, Response *target) {
    int nameLen = countUtf8Char(name);
    if(nameLen < 2 || nameLen > 15) {
        *target = (Response) {0, newString("이름은 2 ~ 15자로 입력해주세요.")};
        return 0;
    } return 1;
}

static int _validateLoginId(String* loginId, Response *target) {
    d("_validateLoginId");
    int idLen = countUtf8Char(loginId);
    if(idLen < 5 || idLen > 15) {
        *target = (Response) {0, newString("아이디는 5 ~ 15자로 입력해주세요.")};
        return 0;
    } return 1;
}

static int _validatePassword(String *password, Response *target) {
    d("_validatePassword");
    int pwLen = countUtf8Char(password);
    if(pwLen < 8 || pwLen > 30) {
        *target = (Response) {0, newString("패스워드는 8 ~ 30자로 입력해주세요.")};
        return 0;
    } return 1;
}