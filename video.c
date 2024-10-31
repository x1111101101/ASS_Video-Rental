#include "video.h"
#include "stdlib.h"
#include "util/debug.h"
#include "util/mystructures.h"


Video* newVideo() {
    Video *video = malloc(sizeof(Video));
    video->id = -1;
    video->articleId = -1;
    video->rentalId = -1;
    video->videoId = newString("");
    video->storedDate = currentDate();
    return video;
}
void freeVideo(Video *video) {
    freeString(video->videoId);
    free(video);
}

static void _finalizer(void *instance) {
    freeVideo((Video*) instance);
}
static int* _idAccessor(void *instance) {
    return &((Video*) instance)->id;
}
static void* _videoIdAccessor(void *instance) {
    return ((Video*) instance)->videoId;
}
static void* _rentalIdAccessor(void *instance) {
    return (void*) ((Video*) instance)->rentalId;
}
static void* _articleIdAccessor(void *instance) {
    return (void*) ((Video*) instance)->articleId;
}

InstanceAccessor VIDEO_ACCESSOR = {_finalizer, _idAccessor};

static void _serialze(FILE *file, void *instance) {
    Video *video = (Video*) instance;
    serializeInt(file, video->id);
    serializeInt(file, video->articleId);
    serializeInt(file, video->rentalId);
    serializeString(file, video->videoId);
    serializeDate(file, video->storedDate);
}

static void* _deserialize(FILE *file) {
    Video *video = malloc(sizeof(Video));
    video->id = deserializeInt(file);
    video->articleId = deserializeInt(file);
    video->rentalId = deserializeInt(file);
    video->videoId = deserializeString(file);
    video->storedDate = deserializeDate(file);
    return video;
}

Serializer VIDEO_SERIALIZER = {_serialze, _deserialize};

static CachedFieldAccessor* _newIntCachedFieldAccessor(FieldAccessor *getter) {
    CachedFieldAccessor *accessor = malloc(sizeof(CachedFieldAccessor));
    accessor->equalsf = EQUALS_INT;
    accessor->hashf = HASH_INT;
    accessor->getter = getter;
    return accessor;
}

DB* newVideoDB() {
    DBCacheManager cacheManager;
    cacheManager.cachedFields = newArrayList(3);
    pushArrayList(cacheManager.cachedFields, _newIntCachedFieldAccessor(_videoIdAccessor));
    pushArrayList(cacheManager.cachedFields, _newIntCachedFieldAccessor(_articleIdAccessor));
    pushArrayList(cacheManager.cachedFields, _newIntCachedFieldAccessor(_rentalIdAccessor));
    DBSearchManager searchManager = {newArrayList(1)};
    pushArrayList(searchManager.fields, _videoIdAccessor);
    DB *db = newDB(VIDEO_ACCESSOR, cacheManager, searchManager);
    return db;
}

static void _serializeDB(FILE *file, void *instance) {
    DB *db = (DB*) instance;
    ArrayList *all = findAllDB(db);
    serializeInt(file, all->size);
    for(int i = 0; i<all->size; i++) {
        VIDEO_SERIALIZER.serialize(file, getArrayList(all, i));
    }
    freeArrayList(all);
}

static void* _deserializeDB(FILE *file) {
    DB *db = newVideoDB();
    int size = deserializeInt(file);
    for(int i = 0; i<size; i++) {
        Video *video = VIDEO_SERIALIZER.deserialize(file);
        insertDB(db, video);
    }
    return db;
}

Serializer VIDEO_DB_SERIALIZER = {_serializeDB, _deserializeDB};

String* toStringVideo(Video *video) {
    StringBuilder *sb = newStringBuilder();
    appendStringBuilder(sb, "id: "); appendIntStringBuilder(sb, video->id); appendlnStringBuilder(sb);
    appendStringBuilder(sb, "articleId: "); appendIntStringBuilder(sb, video->articleId); appendlnStringBuilder(sb);
    appendMStringBuilder(sb, 3, "videoId: ", video->videoId->content, "\n");
    String *str = newString(sb->string);
    freeStringBuilder(sb);
    return str;
}

static int compareVideoId(const void *a, const void *b) {
    Video *p = (Video*) a;
    Video *q = (Video*) b;
    return INT_COMPARATOR((void*) p->id, (void*) q->id);
}

Comparator *VIDEO_ID_COMPARATOR = compareVideoId;