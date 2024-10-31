#ifndef TEXTS_H
#define TEXTS_H
#include <stdint.h>
#include <wchar.h>
#include "mystructures.h"

typedef struct {
    char *content; // UTF-8 formateed char 배열
    int len; // NULL 문자 제외 길이
    int hashcode;
} String;

String* newString(char *content);
String* cloneString(String *string);
void freeString(String *string);
int countUtf8Char(String *string);
unsigned int utf8CharUnicode(char *start);
String* mergeString(char *a, char *b);
extern String *STRING_EMPTY;

/**
 * UTF-8 문자 단위로 부분 문자열 생성
 */
String* subString(String *string, int start, int endExclusive);
String* lowercase(String *string);
EqualsFunc *EQUALS_STRING;
HashFunc *HASH_STRING;
Finalizer *FINALIZER_STRING;


typedef enum Charset {
    UTF8, EUC_KR
} Charset;

Charset detectConsoleCharset();

/**
 * stdin으로 부터 문자열 한 줄을 입력받고 UTF-8 형태로 저장
 * @param target    UTF-8 형태로 저장될 char*
 * @param length    최대 입력 문자열 길이(null 문자 포함). < 1024
 * @return 실제로 사용자가 입력한 문자열 길이
 */
int mygets(char* target, int length);

String *readline();

/**
 * CharIterator
 * UTF-8 문자열을 한 문자씩 순회하는 연산을 지원
 * UTF-8 문자열은 2~4 바이트의 크기이므로 한 문자에 char들을 대응시키는 연산이 복잡하므로 이 구조체를 활용해서 문자열 연산 단순화.
 * 순회 연산이 주 목적이므로 입력된 source 문자열에 대한 메모리 관리는 CharIterator 관련 함수에서 수행하지 않음.
 */
typedef struct {
    const char *source;
    int point;
} CharIterator;

/**
 * CharIterator 생성자
 * @param string    UTF-8 문자열
 */
CharIterator newCharIterator(const char *string);

/**
 * @return nextChar로 조회된 문자들 외에 다른 문자가 더 남아있는지 여부
 */
int hasNextChar(CharIterator *iterator);

/**
 * 다음 문자를 int로 반환. UTF-8의 한 문자 당 할당 크기는 2~4 바이트임에 따라 문자를 통일된 방식으로 접근, 관리하기 위함.
 */
int nextChar(CharIterator *iterator);

/**
 * @param start UTF-8 문자 하나에 해당하는 1~4의 크기를 갖는 char array 
 * @return UTF-8 문자의 할당 byte 수
 */
int utf8len(const char* start);

int asUtf8Char(const char* character);

/**
 * StringBuilder
 */
typedef struct {
    int size, cap;
    char* string;    
} StringBuilder;

StringBuilder* newStringBuilder();
void appendAllStringBuilder(StringBuilder *sb, CharIterator *iterator);
void appendCharStringBuilder(StringBuilder * sb, char c);
void appendUtf8StringBuilder(StringBuilder* sb, int utf8char);
void appendlnStringBuilder(StringBuilder* sb);
void appendStringBuilder(StringBuilder* sb, char *string);
void appendSpaceStringBuilder(StringBuilder *sb);
void appendIntStringBuilder(StringBuilder *sb, int value);
void appendMStringBuilder(StringBuilder *sb, int num, ...);
void printStringBuilder(StringBuilder* sb);
void freeStringBuilder(StringBuilder *sb);


/**
 * wchar_t 문자열 순회용 Iterator
 */
typedef struct {
    wchar_t *string;
    int point;
} WCharIterator;

typedef uint32_t LChar; // UTF-32 문자

/**
 * WCharIterator 생성자
 * @param string    UTF-16 wchar_t 배열
 */
WCharIterator newWCharIterator(wchar_t *string);

/**
 * @return string을 UTF-16으로 해석했을 때 다음 문자 존재 여부.
 */
int hasNextLChar(WCharIterator *iterator);

/**
 * @return UTF-16 문자열의 다음 문자의 UTF-32 값(surrogate 적용)
 */
LChar nextLChar(WCharIterator *iterator);

#endif // Header Guard endif