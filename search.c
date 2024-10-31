#include "search.h"
#include "util/texts.h"
#include "util/mystructures.h"
#include "util/debug.h"
#include "db.h"
#include <stdio.h>
#include <stdlib.h>


SearchHelper* newSearchHelper() {
    SearchHelper *helper = malloc(sizeof(SearchHelper));
    helper->ngram = newHashMap(10, HASH_STRING, EQUALS_STRING);
    return helper;
}

static void _insertNGram(SearchHelper *helper, void *instance, String *name) {
    int len = countUtf8Char(name);
    for(int l = 1; l<=len; l++) {
        CharIterator it = newCharIterator(name->content);
        for(int start = 0; start<=len-l; start++) {
            String *sub = subString(name, start, start+l);
            HashMap *ngram = helper->ngram;
            HashMap *cache = getHashMap(ngram, sub);
            if(cache == NULL) {
                cache = newHashMap(2, HASH_PTR, EQUALS_PTR);
                putHashMap(ngram, sub, cache);
            } else {
                freeString(sub);
            }
            putHashMap(cache, instance, (void*) 1);
        }
    }
}

static void _removeNGram(SearchHelper *helper, void *instance, String *name) {
    d("_removeNGram");
    int len = countUtf8Char(name);
    for(int l = 1; l<=len; l++) {
        CharIterator it = newCharIterator(name->content);
        for(int start = 0; start<=len-l; start++) {
            String *sub = subString(name, start, start+l);
            HashMap *ngram = helper->ngram;
            Entry e = getEntryHashMap(ngram, sub);
            if(e.key == NULL) {
                freeString(sub);
                continue;
            }
            HashMap *cache = (HashMap*) e.value;
            removeHashMap(cache, instance);
            if(cache->size == 0) {
                freeHashMap(cache);
                removeHashMap(ngram, sub);
                freeString((String*) e.key);
            }
            free(sub);
        }
    }
}

void insertSearchHelper(SearchHelper *helper, String *name, void *instance) {
    _insertNGram(helper, instance, name);
    String *lowercaseName = lowercase(name);
    _insertNGram(helper, instance, lowercaseName);
    freeString(lowercaseName);
}

void removeSearchHelper(SearchHelper *helper, String *name, void *instance) {
    d("removeSearchHelper");
    _removeNGram(helper, instance, name);
    String *lowercaseName = lowercase(name);
    _removeNGram(helper, instance, lowercaseName);
    freeString(lowercaseName);
}

static int _compareNGramCaches(const void* a, const void* b) {
    NGramCache **p = (NGramCache**) a;
    NGramCache **q = (NGramCache**) b;
    return (*q)->weight - (*p)->weight;
}
int compareNGramCaches(const void* a, const void* b) {
    return _compareNGramCaches(a, b);
}

static int _ngramWeight(int len) {
    if(len > 5000) len = 5000;
    return len*len*64;
}

static HashMap* _query(SearchHelper *helper, String *name) {
    HashMap *map = newHashMap(32, HASH_PTR, EQUALS_PTR);
    int len = countUtf8Char(name);
    df1("_query - len: %d\n", len);
    for(int l = 1; l<=len; l++) {
        CharIterator it = newCharIterator(name->content);
        int weight = _ngramWeight(l);
        for(int start = 0; start<=len-l; start++) {
            String *sub = subString(name, start, start+l);
            HashMap *cachedMap = getHashMap(helper->ngram, sub);
            freeString(sub);
            if(cachedMap == NULL) continue;
            HashMapIterator it = newHashMapIterator(cachedMap);
            while(hasNextEntry(&it)) {
                Entry e = nextEntry(&it);
                void *instance = e.key;
                NGramCache *cache = malloc(sizeof(NGramCache));
                NGramCache *prevCache = (NGramCache*) getHashMap(map, instance);
                if(prevCache != NULL) free(prevCache);
                putHashMap(map, instance, cache);
                cache->weight = weight;
                cache->instance = instance;
            }
        }
    }
    return map;
}

ArrayList* querySearchHelper(SearchHelper *helper, String *name) {
    df1("querySearchHelper query: [%s]\n", name->content);
    HashMap *map = _query(helper, name);
    String *lowercaseName = lowercase(name);
    HashMap *lowercaseQ = _query(helper, lowercaseName);
    freeString(lowercaseName);
    HashMapIterator it = newHashMapIterator(lowercaseQ);
    while(hasNextEntry(&it)) {
        Entry e = nextEntry(&it);
        NGramCache *cur = (NGramCache*) e.value;
        cur->weight /= 2;
        NGramCache *prev = (NGramCache*) getHashMap(map, e.key);
        if(prev == NULL) {
            putHashMap(map, e.key, cur);
        } else if(prev->weight > cur->weight) {
            free(cur);
        } else {
            free(prev);
            putHashMap(map, e.key, cur);
        }
    }
    freeHashMap(lowercaseQ);
    ArrayList *sorted = newArrayList(map->size + 1);
    it = newHashMapIterator(map);
    while(hasNextEntry(&it)) {
        Entry e = nextEntry(&it);
        pushArrayList(sorted, e.value);
    }
    freeHashMap(map);
    qsort(sorted->arr, sorted->size, sizeof(void*), _compareNGramCaches);
    d("querySearchHelper - complete");
    return sorted;
}