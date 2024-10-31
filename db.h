#ifndef DB_H
#define DB_H
#include "util/mystructures.h"
#include "util/texts.h"
#include "search.h"

/**
 * FieldAccessor
 * @return 구조체의 특정 필드의 값
 */
typedef void*(FieldAccessor)(void *instance);
typedef int*(IdAccessor)(void *instance);

typedef struct {
    Finalizer *finalizer;
    IdAccessor *idAccessor;
} InstanceAccessor;

typedef struct {
    FieldAccessor *getter;
    HashFunc *hashf;
    EqualsFunc *equalsf;
} CachedFieldAccessor;

typedef struct {
    ArrayList *cachedFields; // ArrayList<CachedFieldAccessor> - caching field는 immutable.
} DBCacheManager;

typedef struct {
    ArrayList *fields; // ArrayList<FieldAccessor<String*>*>
} DBSearchManager;

/**
 * DB
 * 구조체 등의 인스턴스 집합을 관리
 * 조회, 추가, 삭제, 검색 지원
 * 
 * 함수에서 리턴되는 ArrayList의 메모리 해제는 호출 측에서 담당해야 함.
 */
typedef struct {
    InstanceAccessor instanceAccessor;
    DBSearchManager searchManager;
    DBCacheManager cacheManager;
    HashMap *idmap;
    HashMap **cache;
    SearchHelper **searchHelpers;
    int lastId;
} DB;

DB *newDB(InstanceAccessor instanceAccessor, DBCacheManager cacheManager, DBSearchManager searchManager);

/**
 * 인스턴스 삽입. 인스턴스의 id가 -1로 설정되어있을 경우 DB가 자동으로 할당
 * @return id
 */
int insertDB(DB *db, void *instance);
void* findByIdDB(DB *db, int id);
ArrayList* findCacheDB(DB *db, int cacheIndex, void *cacheValue);
void* findUniqueCacheDB(DB *db, int cacheIndex, void *cacheValue);

/**
 * 인스턴스 삭제
 * @return 1 if exists else 0
 */
int deleteDB(DB *db, int id);

ArrayList* findAllDB(DB *db);
ArrayList* queryDB(DB *db, int queryIndex, String *name);

#endif