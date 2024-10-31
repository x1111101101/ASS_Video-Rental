#include "user.h"
#include "stdlib.h"
#include "util/debug.h"

static void* _nameAccessor(void *user) {
    return ((User*) user)->name;
};
static void* _loginIdGetter(void *user) {
    return ((User*) user)->loginId;
}
static void* _phoneGetter(void *user) {
    return ((User*) user)->phone;
}
static void _finalizer(void *user) {
    freeUser((User*) user);
}
static int* _idAccessor(void *user) {
    return &((User*) user)->id;
}
static void _serializeUser(FILE *file, void *instance) {
    d("serialize User");
    User *user = (User*) instance;
    serializeInt(file, user->id);
    serializeString(file, user->name);
    serializeString(file, user->phone);
    serializeString(file, user->loginId);
    serializeDate(file, user->birthday);
    serializeInt(file, user->passwordHash);
}
static void* _deserializeUser(FILE *file) {
    User *user = malloc(sizeof(User));
    user->id = deserializeInt(file);
    user->name = deserializeString(file);
    user->phone = deserializeString(file);
    user->loginId = deserializeString(file);
    user->birthday = deserializeDate(file);
    user->passwordHash = deserializeInt(file);
    return user;
}


Serializer USER_SERIALIZER = {_serializeUser, _deserializeUser};
InstanceAccessor USER_ACCESSOR = {_finalizer, _idAccessor};

User* newUser() {
    User *user = malloc(sizeof(User));
    user->name = newString("");
    user->phone = newString("");
    user->loginId = newString("");
    user->passwordHash = 0;
    user->id = -1;
    return user;
}

void freeUser(User* user) {
    freeString(user->name);
    freeString(user->phone);
    freeString(user->loginId);
    free(user);
}

DB* newUserDB() {
    // init CacheManager
    DBCacheManager cacheManager;
    cacheManager.cachedFields = newArrayList(1);
    // loginId field
    CachedFieldAccessor *loginIdAccessor = malloc(sizeof(CachedFieldAccessor));
    loginIdAccessor->getter = _loginIdGetter;
    loginIdAccessor->equalsf = EQUALS_STRING;
    loginIdAccessor->hashf = HASH_STRING;
    pushArrayList(cacheManager.cachedFields, loginIdAccessor);
    // phone field
    CachedFieldAccessor *phoneAccessor = malloc(sizeof(CachedFieldAccessor));
    phoneAccessor->getter = _phoneGetter;
    phoneAccessor->equalsf = EQUALS_STRING;
    phoneAccessor->hashf = HASH_STRING;
    pushArrayList(cacheManager.cachedFields, phoneAccessor);
    // init SearchManager
    DBSearchManager searchManager = {newArrayList(1)};
    pushArrayList(searchManager.fields, _nameAccessor);
    pushArrayList(searchManager.fields, _phoneGetter);
    DB *db = newDB(USER_ACCESSOR, cacheManager, searchManager);
    return db;
}

static void _serializeDB(FILE *file, void *dbInstance) {
    d("serialize User DB");
    DB *db = (DB*) dbInstance;
    ArrayList *all = findAllDB(db);
    serializeInt(file, all->size);
    df1("size: %d\n", all->size);
    for(int i = 0; i<all->size; i++) {
        USER_SERIALIZER.serialize(file, all->arr[i]);
    }
    freeArrayList(all);
}

static void* _deserializeDB(FILE *file) {
    DB *db = newUserDB();
    int size = deserializeInt(file);
    while(size-- > 0) {
        User *user = USER_SERIALIZER.deserialize(file);
        insertDB(db, user);
    }
    return db;
}

Serializer USER_DB_SERIALIZER = {_serializeDB, _deserializeDB};