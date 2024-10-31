#include "rental.h"
#include <stdlib.h>
#include "util/texts.h"

Rental *newRental() {
    Rental *rental = malloc(sizeof(Rental));
    rental->id = -1;
    rental->userId = -1;
    rental->videoId = -1;
    rental->rentTime = -1;
    rental->returnTime = -1;
    return rental;
}
void freeRental(Rental *rental) {
    free(rental);
}
static int* _idAccessor(void *instance) {
    return &((Rental*) instance)->id;
}
static void _finalizer(void *instance) {
    freeRental((Rental*) instance);
}
static void* _userIdAccessor(void *instance) { return (void*)((Rental*) instance)->userId; }
static void* _videoIdAccessor(void *instance) { return (void*)((Rental*) instance)->videoId; }

static CachedFieldAccessor* _newIntCachedFieldAccessor(FieldAccessor *getter) {
    CachedFieldAccessor *accessor = malloc(sizeof(CachedFieldAccessor));
    accessor->equalsf = EQUALS_INT;
    accessor->hashf = HASH_INT;
    accessor->getter = getter;
    return accessor;
}

DB* newRentalDB() {
    DBCacheManager cacheManager = {newArrayList(2)};
    pushArrayList(cacheManager.cachedFields, _newIntCachedFieldAccessor(_userIdAccessor));
    pushArrayList(cacheManager.cachedFields, _newIntCachedFieldAccessor(_videoIdAccessor));
    DBSearchManager searchManager = {newArrayList(1)};
    DB *db = newDB(RENTAL_ACCESSOR, cacheManager, searchManager);
}

int isReturnedRental(Rental *rental) {
    return rental->returnTime != 0;
}

InstanceAccessor RENTAL_ACCESSOR = {_finalizer, _idAccessor};

static void _serialze(FILE *file, void *instance) {
    Rental *rental = (Rental*) instance;
    serializeInt(file, rental->id);
    serializeInt(file, rental->userId);
    serializeInt(file, rental->videoId);
    serializeInt(file, rental->rentTime);
    serializeInt(file, rental->returnTime);
}

static void* _deserialize(FILE *file) {
    Rental *rental = malloc(sizeof(Rental));
    rental->id = deserializeInt(file);
    rental->userId = deserializeInt(file);
    rental->videoId = deserializeInt(file);
    rental->rentTime = deserializeInt(file);
    rental->returnTime = deserializeInt(file);
    return rental;
}

static void _serializeDB(FILE *file, void *instance) {
    DB *db = instance;
    ArrayList *all = findAllDB(db);
    serializeInt(file, all->size);
    for(int i = 0; i<all->size; i++) {
        RENTAL_SERIALIZER.serialize(file, all->arr[i]);
    }
    freeArrayList(all);
}

static void* _deserializeDB(FILE *file) {
    DB *db = newRentalDB();
    int size = deserializeInt(file);
    for(int i = 0; i<size; i++) {
        insertDB(db, RENTAL_SERIALIZER.deserialize(file));
    }
    return db;
}
static int _cmpId(const void *a, const void *b) {
    const Rental *p = a;
    const Rental *q = b;
    return q->id - p->id;
}
Comparator *RENTAL_ID_COMPARATOR = _cmpId;
Serializer RENTAL_SERIALIZER = { _serialze, _deserialize };
Serializer RENTAL_DB_SERIALIZER = {_serializeDB, _deserializeDB};