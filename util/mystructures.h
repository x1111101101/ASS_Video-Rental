#ifndef MYST_H
#define MYST_H

/**
 * Finalizer
 * 인스턴스의 메모리 할당을 해제하는 함수
 */
typedef void(Finalizer)(void *instance);

typedef int(Comparator)(const void *a, const void *b);
extern Comparator *INT_COMPARATOR;

/*******************************************************************************************************
 * ArrayList
 * 동적 할당으로 크기 자동 조절하는 스택
 */
typedef struct {
    int size, cap;
    void **arr;
} ArrayList;

ArrayList* newArrayList(int initialCapability);
void* getArrayList(ArrayList *list, int index);
void pushArrayList(ArrayList *list, void *value);
void removeAtArrayList(ArrayList *list, int index);
int containsArrayList(ArrayList *list, void *value);
void sortArrayList(ArrayList *list, Comparator *comparator);
void freeArrayList(ArrayList *instance);


/*******************************************************************************************************
 *  * HashMap
 * 기본적인 Hash Table 구현.
 * capacity 자동 증가, capacity개의 Linked List 사용.
 * O(1)의 조회, 생성, 삭제, 수정 시간복잡도.
 * 
 * key, value에 대한 메모리 관리는 해시맵 외부에서 수행해야한다.
 * key는 Immutable한 인스턴스를 사용해야한다.
 *
 */

/**
 * Entry
 * HashMap 상의 하나의 key-value 쌍
 */
typedef struct {
    void *key, *value;
} Entry;

/**
 * EqualsFunc
 * 다른 key를 사용하더라도 hash 충돌이 발생할 수 있다. hash가 같은 key 끼리 비교하는 함수.
 * @param a key a
 * @param b key b
 * @return 두개의 Key instance가 동일한 값을 나타내는지
 */
typedef int(EqualsFunc)(void *a, void *b);

/**
 * HashFunc
 * key instance에 대한 hash 함수
 */
typedef int(HashFunc)(void *a);

HashFunc *HASH_INT;
HashFunc *HASH_PTR;
EqualsFunc *EQUALS_INT;
EqualsFunc *EQUALS_PTR;

/**
 * HashMap 구현에 쓰이는 노드
 */
typedef struct HNode{
    int hash;
    void *key;
    void *value;
    struct HNode *next;
} HNode;

/**
 * HashMap 구조체.
 * table: HNode(Linked List) 포인터들의 배열. 한 index에 해당하는 값이 NULL일 경우 빈 공간을 뜻함.
 * cap: table의 크기
 * size: 삽입된 key의 개수
 */
typedef struct {
    int size, cap;
    HNode **table;
    HashFunc *hashf;
    EqualsFunc *equalsf;
} HashMap;

HashMap* newHashMap(int initialCapacity, HashFunc *hashf, EqualsFunc *equalsf);
void putHashMap(HashMap *map, void *key, void *value);
Entry removeHashMap(HashMap *map, void *key);
void* getHashMap(HashMap *map, void *key);
Entry getEntryHashMap(HashMap *map, void *key);
void freeHashMap(HashMap *map);


// 아래는 메모리 관련 로직 작성의 편의를 위한 매크로성 함수들 선언
/**
 * getfHashMap
 * map의 노드에 저장된 key의 주소가 파라미터 newKey의 주소와 동일하지 않을 경우
 * keyFinalizer를 이용해서 기존 key의 메모리를 해제한다. 그리고 HNode 상의 key는 newKey로 대체된다.
 * @return value or NULL
 */
void* getfHashMap(HashMap *map, void *newKey, Finalizer *keyFinalizer);

/**
 * putfHashMap
 * map의 노드에 저장된 key의 주소가 파라미터 newKey의 주소와 동일하지 않을 경우
 * keyFinalizer를 이용해서 기존 key의 메모리를 해제한다. 그리고 HNode 상의 key는 newKey로 대체된다.
 * @return previous value or NULL
 */
void* putfHashMap(HashMap *map, void *newKey, void *value, Finalizer *keyFinalizer);



/*******************************************************************************************************
 * HashMapIterator
 * HashMap 순회를 위한 구조체, 함수
 */
typedef struct {
    HashMap *map;
    int idx;
    HNode* node;
} HashMapIterator;

HashMapIterator newHashMapIterator(HashMap *map);
int hasNextEntry(HashMapIterator *iterator);
Entry nextEntry(HashMapIterator *iterator);

#endif