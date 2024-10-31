#include <stdio.h>
#include <stdlib.h>
#include "mystructures.h"

void throwMyStructureException(char* msg) {
    printf("error: %s", msg);
    printf("enter any key to exit");
    char buf[2];
    fgets(buf, 2, stdin);
    exit(1);
}

static int _intComparator(const void *a, const void *b) {
    return *((int*) a) - *((int*) b);
}
Comparator *INT_COMPARATOR = _intComparator;

/***************************************************
 * HashMap
 */
static void checkResizeHashMap(HashMap *map);
static void _putHashMap(EqualsFunc *eqf, HashFunc *hashf, HNode **table, int cap, void *key, void *value);

HNode* newHNode() {
    HNode *node = malloc(sizeof(HNode));
    node->next = NULL;
    return node;
}

HashMap* newHashMap(int initialCapacity, HashFunc *hashf, EqualsFunc *equalsf) {
    HashMap *map = malloc(sizeof(HashMap));
    map->hashf = hashf;
    map->equalsf = equalsf;
    map->cap = initialCapacity;
    map->table = malloc(sizeof(HNode*) * initialCapacity);
    map->size = 0;
    for(int i = 0; i<map->cap; i++) map->table[i] = NULL;
    return map;
}

static HNode* _findNode(HashMap *map, void *key, int hash, HNode *head) {
    HNode *node = head;
    while(node) {
        if(node->hash == hash && map->equalsf(key, node->key)) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

/**
 * @param node  Nullable
 */
static Entry _findEntry(EqualsFunc *equalsf, HNode *node, void *key, int hash) {
    Entry entry = {NULL, NULL};
    while(node) {
        if(node->hash == hash && equalsf(node->key, key)) {
            entry.key = node->key;
            entry.value = node->value;
            return entry;
        }
        node = node->next;
    }
    return entry;
}

static void _putHashMap(EqualsFunc *eqf, HashFunc *hashf, HNode **table, int cap, void *key, void *value) {
    int hash = hashf(key);
    int internalHash = ((unsigned int ) hash) % cap;
    HNode *temp = table[internalHash];
    HNode *node = newHNode();
    node->hash = hash;
    node->key = key;
    node->value = value;
    table[internalHash] = node;
    node->next = temp;
}

void putHashMap(HashMap *map, void *key, void *value) {
    removeHashMap(map, key);
    checkResizeHashMap(map);
    _putHashMap(map->equalsf, map->hashf, map->table, map->cap, key, value);
    map->size++;
}

static void _freeNodes(HNode **table, int cap) {
    for (int i = 0; i < cap; i++) {
        HNode *node = table[i];
        while (node != NULL) {
            free(node);
            node = node->next;
        }
    }
}

static void checkResizeHashMap(HashMap *map) {
    if(map->size <= map->cap * 8)
        return;
    
    int newCap = map->cap * 2;
    HNode **newTable = malloc(sizeof(HNode) * newCap);
    for(int i = 0; i<newCap; i++) newTable[i] = NULL;
    HashMapIterator it = newHashMapIterator(map);
    while(hasNextEntry(&it)) {
        Entry next = nextEntry(&it);
        _putHashMap(map->equalsf, map->hashf, newTable, newCap, next.key, next.value);
    }
    _freeNodes(map->table, map->cap);
    free(map->table);
    map->table = newTable;
    map->cap = newCap;
}

Entry removeHashMap(HashMap *map, void *key) {
    int hash = map->hashf(key);
    int internalHash = ((unsigned int) hash) % map->cap;
    HNode *node = map->table[internalHash];
    HNode *prev = NULL;
    Entry result = {NULL, NULL};
    while(node) {
        if(node->hash != hash || !map->equalsf(node->key, key)) {
            prev = node;
            node = node->next;
            continue;
        }
        void *value = node->value;
        if(prev == NULL) {
            map->table[internalHash] = node->next;
            result.key = node->key;
            result.value = node->value;
            free(node);
            map->size--;
            return result;
        }
        prev->next = node->next;
        result.key = node->key;
        result.value = node->value;
        free(node);
        map->size--;
        return result;
    }
    return result;
}

void* getHashMap(HashMap *map, void *key) {
    int hash = map->hashf(key);
    int internalHash = ((unsigned int) hash) % map->cap;
    HNode *node = map->table[internalHash];
    while(node) {
        if(node->hash == hash && map->equalsf(node->key, key)) 
            return node->value;
        node = node->next;
    }
    return NULL;
}

Entry getEntryHashMap(HashMap *map, void *key) {
    int hash = map->hashf(key);
    int internalHash = ((unsigned int) hash) % map->cap;
    return _findEntry(map->equalsf, map->table[internalHash], key, hash);
}

void freeHashMap(HashMap *map) {
    _freeNodes(map->table, map->cap);
    free(map->table);
    free(map);
}

static void _findNextEntry(HashMapIterator *it) {
    HashMap *map = it->map;
    if(it->node && it->node->next) {
        it->node = it->node->next;
        return;
    }
    while(++(it->idx) < map->cap) {
        if(map->table[it->idx]) {
            it->node = map->table[it->idx];
            return;
        }
    }
    it->node = NULL;
    return;
}

void* getfHashMap(HashMap *map, void *newKey, Finalizer *keyFinalizer) {
    int hash = map->hashf(newKey);
    int internalHash = ((unsigned int) hash) % map->cap;
    HNode *node = _findNode(map, newKey, hash, map->table[internalHash]);
    if(!node) return NULL;
    if(node->key != newKey) {
        keyFinalizer(node->key);
    }
    return node->value;
}

void* putfHashMap(HashMap *map, void *newKey, void *value, Finalizer *keyFinalizer) {
    int hash = map->hashf(newKey);
    int internalHash = ((unsigned int) hash) % map->cap;
    HNode *node = _findNode(map, newKey, hash, map->table[internalHash]);
    if(!node) {
        HNode *new = newHNode();
        new->hash = hash;
        new->value = value;
        new->key = newKey;
        new->next = map->table[internalHash];
        map->table[internalHash] = new;
        return NULL;
    }
    if(node->key != newKey) {
        keyFinalizer(node->key);
        node->key = newKey;
    }
    void *prev = node->value;
    node->value = value;
    return prev;
}

int _hashInt(void *p) {return (int) p;}
int _equalsInt(void *a, void *b) {return a == b;}
int _hashPtr(void *p) {return (int) p;}
int _equalsPtr(void *a, void *b) { return a == b; }

HashFunc *HASH_INT = _hashInt;
EqualsFunc *EQUALS_INT = _equalsInt;
HashFunc *HASH_PTR = _hashPtr;
EqualsFunc *EQUALS_PTR = _equalsPtr;


/***************************************************
 * HashMapIterator
 * 가벼운 구조체라 동작 할당을 쓰지 않음.
 * 메모리 관리 불필요.
 */
HashMapIterator newHashMapIterator(HashMap *map) {
    HashMapIterator it;
    it.idx = 0;
    it.map = map;
    it.node = map->table[0];
    if(it.node == NULL) {
        _findNextEntry(&it);
    }
    return it;
}
int hasNextEntry(HashMapIterator *it) {
    return it->node != NULL;
}
Entry nextEntry(HashMapIterator *it) {
    HNode *node = it->node;
    _findNextEntry(it);
    return (Entry) {node->key, node->value};
}


/***************************************************
 * ArrayList
 */
ArrayList* newArrayList(int initialCapability) {
    if(initialCapability <= 0) initialCapability = 1;
    ArrayList* instance = malloc(sizeof(ArrayList));
    instance->size = 0;
    instance->cap = initialCapability;
    instance->arr = malloc(sizeof(void*) * initialCapability);
    return instance;
}

void* getArrayList(ArrayList *list, int index) {
    if(index < 0 || index >= list->size) {
        throwMyStructureException("index out of bounds");
    }
    return list->arr[index];
}

void pushArrayList(ArrayList *list, void *value) {
    if(list->cap == list->size) {
        list->cap *= 2;
        list->arr = realloc(list->arr, sizeof(void*) * (size_t) list->cap);
    }
    void **arr = list->arr;
    arr[list->size++] = value;
}

void removeAtArrayList(ArrayList *list, int index) {
    if(index < 0 || index >= list->size) throwMyStructureException("index out of bounds");
    for(int i = index; i<list->size; i++) list->arr[i] = list->arr[i+1];
    list->size--;
}

void sortArrayList(ArrayList *list, Comparator *comparator) {
    qsort(list->arr, list->size, sizeof(void*), comparator);
}

int containsArrayList(ArrayList *list, void *value) {
    for(int i = 0; i<list->size; i++) if(list->arr[i] == value) return 1;
    return 0;
}

void freeArrayList(ArrayList *instance) {
    free(instance->arr);
    free(instance);
}