#include "article.h"
#include "util/texts.h"
#include "util/serialization.h"
#include <stdio.h>
#include <stdlib.h>

Article* newArticle() {
    Article *a = malloc(sizeof(Article));
    a->description = newString("");
    a->name = newString("");
    a->rating = ERATING_ALL;
    a->category = ECATEGORY_ETC;
    a->id = -1;
    return a;
}
Article* cloneArticle(Article *article) {
    Article *a = malloc(sizeof(Article));
    a->name = newString(article->name->content);
    a->description = newString(article->description->content);
    a->rating = article->rating;
    a->category = article->category;
    a->id = article->id;
    return a;
}
void freeArticle(Article *article) {
    freeString(article->name);
    freeString(article->description);
    free(article);
}
static int* _idAccessor(void *article) {
    return &((Article*) article)->id;
}
static void* _nameAccessor(void *article) {
    return ((Article*) article)->name;
}
static void* _categoryGetter(void *article) {
    return (void*) ((Article*) article)->category;
}

InstanceAccessor ARTICLE_ACCESSOR = { (Finalizer*) freeArticle, _idAccessor };

static void serialize(FILE *file, void *instance) {
    Article *article = instance;
    serializeInt(file, article->id);
    serializeInt(file, article->category);
    serializeInt(file, article->rating);
    serializeString(file, article->name);
    serializeString(file, article->description);
}

static void* deserialize(FILE *file) {
    Article *article = malloc(sizeof(Article));
    article->id = deserializeInt(file);
    article->category = deserializeInt(file);
    article->rating = deserializeInt(file);
    article->name = deserializeString(file);
    article->description = deserializeString(file);
    return article;
}

static void serializeDB(FILE *file, void *instance) {
    DB *articles = instance;
    ArrayList *all = findAllDB(articles);
    serializeInt(file, all->size);
    for(int i = 0; i<all->size; i++) {
        ARTICLE_SERIALIZER.serialize(file, all->arr[i]);
    }
    freeArrayList(all);
}

static void* deserializeDB(FILE *file) {
    DB *db = newArticleDB();
    int size = deserializeInt(file);
    for(int i = 0; i<size; i++) {
        insertDB(db, ARTICLE_SERIALIZER.deserialize(file));
    }
    return db;
}

Serializer ARTICLE_SERIALIZER = {serialize, deserialize};
Serializer ARTICLE_DB_SERIALIZER = {serializeDB, deserializeDB};


DB* newArticleDB() {
    DBCacheManager cacheManager = {newArrayList(1)};
    // Cache Name
    CachedFieldAccessor *accessor = malloc(sizeof(CachedFieldAccessor));
    accessor->equalsf = EQUALS_STRING;
    accessor->hashf = HASH_STRING;
    accessor->getter = _nameAccessor;
    pushArrayList(cacheManager.cachedFields, accessor);
    // Cache Category
    accessor = malloc(sizeof(CachedFieldAccessor));
    accessor->equalsf = EQUALS_INT;
    accessor->hashf = HASH_INT;
    accessor->getter = _categoryGetter;
    pushArrayList(cacheManager.cachedFields, accessor);
    // SearchManager
    DBSearchManager searchManager = {newArrayList(1)};
    pushArrayList(searchManager.fields, _nameAccessor);
    DB *db = newDB(ARTICLE_ACCESSOR, cacheManager, searchManager);
    return db;
}

char CATEGORY_NAMES[][16] = {
    "🥷🏽 액션",
    "🧸 코미디",
    "📡 SF",
    "🎵 음악",
    "⚔️ 전쟁",
    "🩻 느와르",
    "💘 로맨스",
    "기타"
};

char RATING_NAMES[][16] = {
    "전체연령가",
    "12세 이상",
    "15세 이상",
    "19세 이상"
};