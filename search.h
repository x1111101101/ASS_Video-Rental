#ifndef SEARCH_H
#define SEARCH_H

#include "util/texts.h"
#include "util/mystructures.h"

typedef struct {
    HashMap *ngram; // HashMap<String, HashMap<void*, int>>
} SearchHelper;

typedef struct {
    void *instance;
    int weight;
} NGramCache;

SearchHelper* newSearchHelper();
void insertSearchHelper(SearchHelper *helper, String *name, void *instance);
void removeSearchHelper(SearchHelper *helper, String *name, void *instance);
ArrayList* querySearchHelper(SearchHelper *helper, String *name);
int compareNGramCaches(const void* a, const void* b);

#endif