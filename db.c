#include "db.h"
#include "util/texts.h"
#include "util/debug.h"
#include <stdlib.h>
#include <stdio.h>
#include "search.h"

#define DB_MAX_CACHES  (16)

static void throwDBException(char *msg) {
    printf(msg);
    getchar();
    exit(1);
}

static HashMap* _findCacheTarget(DB *db, int cacheIndex, void* cacheValue) {
    CachedFieldAccessor *accessor = db->cacheManager.cachedFields->arr[cacheIndex];
    HashMap *cache = db->cache[cacheIndex];
    HashMap *target = getHashMap(cache, cacheValue);
    if(target == NULL) {
        target = newHashMap(2, HASH_INT, EQUALS_INT);
        putHashMap(cache, (void*) cacheValue, target);
    }
    return target;
}
static HashMap* _findCacheTargetOrNull(DB *db, int cacheIndex, void* cacheValue) {
    d("_findCacheTargetOrNull\n");
    CachedFieldAccessor *accessor = db->cacheManager.cachedFields->arr[cacheIndex];
    HashMap *cache = db->cache[cacheIndex];
    HashMap *target = getHashMap(cache, cacheValue);
    //df1("cacheIdx: %d\n", cacheIndex);
    //df1("cacheTarget: %p\n", target);
    return target;
}

static int _getId(DB *db, void *instance) {
    int id = *((int*)db->instanceAccessor.idAccessor(instance));
    return id;
}

static String* _getName(DB *db, int queryIndex, void *instance) {
    d("_getName");
    FieldAccessor *fieldAccessor = (FieldAccessor*) db->searchManager.fields->arr[queryIndex];
    return (String*) fieldAccessor(instance);
}

static int _allocateId(DB *db, void *instance) {
    InstanceAccessor accessor = db->instanceAccessor;
    int *field = accessor.idAccessor(instance);
    *field = db->lastId++;
    return *field;
}

DB *newDB(InstanceAccessor instanceAccessor, DBCacheManager cacheManager, DBSearchManager searchManager) {
    DB *db = malloc(sizeof(DB));
    db->instanceAccessor = instanceAccessor;
    db->cacheManager = cacheManager;
    db->idmap = newHashMap(32, HASH_INT, EQUALS_INT);
    db->cache = malloc(sizeof(void*) * cacheManager.cachedFields->size);
    db->searchManager = searchManager;
    db->searchHelpers = malloc(sizeof(void*) * searchManager.fields->size);
    for(int i = 0; i<cacheManager.cachedFields->size; i++) {
        CachedFieldAccessor *accessor = (CachedFieldAccessor*) cacheManager.cachedFields->arr[i];
        db->cache[i] = newHashMap(32, accessor->hashf, accessor->equalsf);
    }
    for(int i = 0; i<searchManager.fields->size; i++) db->searchHelpers[i] = newSearchHelper();
    db->lastId = 0;
}

int insertDB(DB *db, void *instance) {
    d("insertDB Called");
    df1("lastId: %d\n", db->lastId);
    int cacheValues[DB_MAX_CACHES];
    int id = _getId(db, instance);
    if(id == -1) {
        d("id allocation");
        id = _allocateId(db, instance);
    } else if(id >= db->lastId) db->lastId = id+1;
    df1("id: %d\n", id);
    if(findByIdDB(db, id) != NULL) {
        throwDBException("duplicated id");
    }
    d("id checked\n");
    DBCacheManager cacheManager = db->cacheManager;
    for(int i = 0; i<cacheManager.cachedFields->size; i++) {
        df1("cache: %d\n", i);
        CachedFieldAccessor *accessor = cacheManager.cachedFields->arr[i];
        void *value = accessor->getter(instance);
        HashMap *cacheTarget = _findCacheTarget(db, i, value);
        putHashMap(cacheTarget, (void*) id, instance);
    }
    putHashMap(db->idmap, (void*) id, instance);
    // 검색 등록
    d("query insert\n");
    DBSearchManager searchManager = db->searchManager;
    for(int i = 0; i<searchManager.fields->size; i++) {
        df1("QI: %d\n", i);
        SearchHelper *helper = db->searchHelpers[i];
        String *name = _getName(db, i, instance);
        insertSearchHelper(helper, name, instance);
    }
    d("complete: query insert");
    return id;
}
void* findByIdDB(DB *db, int id) {
    return getHashMap(db->idmap, (void*) id);
}
ArrayList* findCacheDB(DB *db, int cacheIndex, void* cacheValue) {
    HashMap *cacheTarget = _findCacheTargetOrNull(db, cacheIndex, cacheValue);
    if(cacheTarget == NULL) return newArrayList(0);
    ArrayList *list = newArrayList(cacheTarget->size);
    HashMapIterator it = newHashMapIterator(cacheTarget);
    while(hasNextEntry(&it)) {
        Entry e = nextEntry(&it);
        pushArrayList(list, e.value);
    }
    return list;
}

void* findUniqueCacheDB(DB *db, int cacheIndex, void *cacheValue) {
    ArrayList *list = findCacheDB(db, cacheIndex, cacheValue);
    void *result = NULL;
    if(list->size > 0) result = list->arr[0];
    freeArrayList(list);
    return result;
}

ArrayList* findAllDB(DB *db) {
    HashMap *map = db->idmap;
    ArrayList *list = newArrayList(map->size);
    HashMapIterator it = newHashMapIterator(map);
    while(hasNextEntry(&it)) {
        Entry e = nextEntry(&it);
        pushArrayList(list, e.value);
    }
    return list;
}

ArrayList* queryDB(DB *db, int queryIndex, String *name) {
    df1("queryDB: [%s]\n", name->content);
    SearchHelper *helper = (SearchHelper*) db->searchHelpers[queryIndex];
    return querySearchHelper(helper, name);
}

int deleteDB(DB *db, int id) {
    df1("deleteDB: %d\n", id);
    void *instance = getHashMap(db->idmap, (void*) id);
    if(instance == NULL) return 0;
    DBCacheManager cacheManager = db->cacheManager;
    for(int i = 0; i<cacheManager.cachedFields->size; i++) {
        CachedFieldAccessor *accessor = (CachedFieldAccessor*) cacheManager.cachedFields->arr[i];
        void *value = accessor->getter(instance);
        HashMap *map = db->cache[i];
        HashMap *target = getHashMap(map, value);
        removeHashMap(target, (void*) id);
        if(target->size == 0) {
            freeHashMap(target);
            removeHashMap(map, value);
        }
    }
    DBSearchManager searchManager = db->searchManager;
    for(int i = 0; i<searchManager.fields->size; i++) {
        FieldAccessor *accessor = searchManager.fields->arr[i];
        String *name = (String*) accessor(instance);
        removeSearchHelper(db->searchHelpers[i], name, instance);
    }
    removeHashMap(db->idmap, (void*) id);
    db->instanceAccessor.finalizer(instance);
    return 1;
}