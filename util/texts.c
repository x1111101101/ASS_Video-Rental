#include "texts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if defined(_WIN32) || defined(_WIN64)
// WINDOWS
#include <wchar.h>
#include <locale.h>
#include <windows.h>

#define MAX_BUFFER_SIZE (1024)

static const int UTF8_LENTH[256] = {
    [0x00 ... 0x7F] = 1,
    [0xC0 ... 0xDF] = 2,
    [0xE0 ... 0xEF] = 3,
    [0xF0 ... 0xF7] = 4,
};

void throwTextsException(char* msg) {
    printf(msg);
    getchar();
    exit(1);
}

Charset detectConsoleCharset() {
    char *cur = setlocale(LC_CTYPE, NULL);
    if(cur && strstr(cur, "UTF-8")) {
        return UTF8;
    }
    return EUC_KR;
}

void eucKrToUTF16(const char *input, wchar_t *output) {
    setlocale(LC_ALL, ""); // mbrtowc 호출 시 UTF-16 형태로 변환되도록 설정
    size_t i = 0;
    mbstate_t state = {0};
    size_t output_index = 0;
    while (*input) {
        size_t len = mbrtowc(&output[output_index], input, MB_CUR_MAX, &state);
        if (len == (size_t)-1 || len == (size_t)-2) {
            throwTextsException("wrong UTF-16 format");
        }
        input += len;
        output_index++;
    }
    output[output_index] = L'\0';
}

int utf32ToUTF8(LChar utf32Char) {
    int result = 0;
    char *utf8Str = (char*) &result;
    if (utf32Char <= 0x7F) {
        utf8Str[0] = utf32Char & 0x7F;
    } else if (utf32Char <= 0x7FF) {
        utf8Str[0] = 0xC0 | ((utf32Char >> 6) & 0x1F);
        utf8Str[1] = 0x80 | (utf32Char & 0x3F);
    } else if (utf32Char <= 0xFFFF) {
        utf8Str[0] = 0xE0 | ((utf32Char >> 12) & 0x0F);
        utf8Str[1] = 0x80 | ((utf32Char >> 6) & 0x3F);
        utf8Str[2] = 0x80 | (utf32Char & 0x3F);
    } else if (utf32Char <= 0x10FFFF) {
        utf8Str[0] = 0xF0 | ((utf32Char >> 18) & 0x07);
        utf8Str[1] = 0x80 | ((utf32Char >> 12) & 0x3F);
        utf8Str[2] = 0x80 | ((utf32Char >> 6) & 0x3F);
        utf8Str[3] = 0x80 | (utf32Char & 0x3F);
    }
    return result;
}

/**
 * mygets Windows OS 구현
 * Windows에서는 System Default로 Console 입력이 한국어 사용자의 경우 EUC-KR 인코딩으로 처리됨.
 * EUC-KR -> wchar_t(UTF-16) ---(surrogate)---> UTF-32 -> UTF-8로 변환
 */
int mygets(char* target, int length) {
    char prevLocale[1024] = "C";
    char *currentLocale = setlocale(LC_ALL, NULL);
    if(currentLocale) {
        strcpy(prevLocale, currentLocale);
    }
    setlocale(LC_ALL, "");
    char input[MAX_BUFFER_SIZE];
    wchar_t output[MAX_BUFFER_SIZE*2];
    fgets(input, sizeof(input), stdin);
    int inputLen = strcspn(input, "\n");
    input[inputLen] = '\0';
    if(inputLen == 0) {
        setlocale(LC_ALL, prevLocale);
        return 0;
    }
    Charset charset = detectConsoleCharset();
    if(charset == UTF8) {
        for(int i = 0; i<length-1; i++) target[i] = input[i];
        input[length-1] = '\0';
        return inputLen;
    }
    // EUC-KR --> UTF-16 변환
    eucKrToUTF16(input, output);
    setlocale(LC_ALL, prevLocale);
    StringBuilder *sb = newStringBuilder();
    WCharIterator it = newWCharIterator(output);
    while(hasNextLChar(&it)) {
        // UTF-16 --> UTF-32 변환
        LChar next = nextLChar(&it);
        // UTF-32 --> UTF-8 변환
        int utf8Char = utf32ToUTF8(next);
        appendUtf8StringBuilder(sb, utf8Char);
    }
    target[0] = '\0';
    CharIterator cit = newCharIterator(sb->string);
    int i = 0;
    while(hasNextChar(&cit)) {
        int next = nextChar(&cit);
        int len = utf8len((const char*) &next);
        if(i >= length-4) break;
        for(int k = 0; k<len; k++) {
            target[i] = sb->string[i];
            i++;
        }
    }
    target[i] = '\0';
    freeStringBuilder(sb);
    return inputLen;
}
#else
// LINUX
void mygets(char* target, int length) {
    fgets(target, length, stdin);
    input[strcspn(input, "\n")] = '\0';
}
#endif

String* readline() {
    static char buf[MAX_BUFFER_SIZE];
    buf[0] = '\0';
    mygets(buf, 1024);
    return newString(buf);
}

/**
 * String
 */
String* subString(String *string, int start, int endExclusive) {
    CharIterator it = newCharIterator(string->content);
    for(int i = 0; i<start; i++) nextChar(&it);
    StringBuilder *sb = newStringBuilder();
    for(int i = start; i<endExclusive; i++) appendUtf8StringBuilder(sb, nextChar(&it));
    String *sub = newString(sb->string);
    freeStringBuilder(sb);
    return sub;
}

void _stirngFinalize(void *string) {
    freeString((String*) string);
}

int _stringHash(void *string) {
    return ((String*) string)->hashcode;
}

int _stringEquals(void *s1, void *s2) {
    String *a = (String*) s1;
    String *b = (String*) s2;
    if(a->hashcode != b->hashcode || a->len != b->len) return 0;
    for(int i = 0; i<a->len; i++) if(a->content[i] != b->content[i]) return 0;
    return 1;
}

String *STRING_EMPTY;
__attribute__((constructor))
static void _init() {
    STRING_EMPTY = newString("");
}

EqualsFunc *EQUALS_STRING = _stringEquals;
HashFunc *HASH_STRING = _stringHash;
Finalizer *FINALIZER_STRING = _stirngFinalize;

int hashString(char *content, int len) {
    int hash = 0;
    for(int i = 0; i<len; i++) {
        hash <<= 3;
        hash += content[i] << (i%7);
    }
    return hash;
}

String *newString(char *content) {
    String *string = malloc(sizeof(String));
    int len = 0;
    CharIterator it = newCharIterator(content);
    while(hasNextChar(&it)) {
        int next = nextChar(&it);
        len += utf8len((const char*) &next);
    }
    it.point = 0;
    string->content = malloc(sizeof(char) * (len + 1));
    string->len = len;
    string->hashcode = hashString(content, len);
    for(int i = 0; i<len; i++) string->content[i] = content[i];
    string->content[len] = '\0';
    return string;
}
String* cloneString(String *string) {
    return newString(string->content);
}
int countUtf8Char(String *string) {
    CharIterator it = newCharIterator(string->content);
    int len = 0;
    while(hasNextChar(&it)) {
        nextChar(&it);
        len++;
    }
    return len;
}
static int _extractChoseong(unsigned int unicode) {
    const char choseongs[][4] = {
        "ㄱ", "ㄲ", "ㄴ", "ㄷ", "ㄸ", "ㄹ", "ㅁ", "ㅂ", "ㅃ", "ㅅ", "ㅆ",
        "ㅇ", "ㅈ", "ㅉ", "ㅊ", "ㅋ", "ㅌ", "ㅍ", "ㅎ"
    };
    unsigned int index = unicode - 0xAC00;
    int choseongIndex = index / (21 * 28);
    CharIterator it = newCharIterator(choseongs[choseongIndex]);
    return nextChar(&it);
}

String* lowercase(String *string) {
    CharIterator it = newCharIterator(string->content);
    StringBuilder *sb = newStringBuilder();
    while(hasNextChar(&it)) {
        int next = nextChar(&it);
        if('A' <= next && next <= 'Z') {
            appendCharStringBuilder(sb, next + ('a'-'A'));
            continue;
        }
        unsigned unicode = utf8CharUnicode((char*) &next);
        if(0xAC00 <= unicode && unicode <= 0xD7A3) { // 가 ~ 힣
            appendUtf8StringBuilder(sb, _extractChoseong(unicode));
            continue;
        }
        appendUtf8StringBuilder(sb, next);
    }
    String *result = newString(sb->string);
    freeStringBuilder(sb);
    return result;
}

static void _freeString(String *string) {
    free(string->content);
    free(string);
}

#define STR_FREE_THRESHOLD  (64)
void freeString(String *string) {
    static ArrayList* list = NULL;
    if(list == NULL) list = newArrayList(STR_FREE_THRESHOLD);
    pushArrayList(list, string);
    if(list->size == STR_FREE_THRESHOLD) {
        for(int i = 0; i<list->size; i++) _freeString((String*) list->arr[i]);
        list->size = 0; 
    }
}

/**
 * CharIterator
 */

int utf8len(const char* start) {
    int len = UTF8_LENTH[(unsigned char) start[0]];
    if(!len) throwTextsException("wrong utf8 format");
    return len;
}

unsigned int utf8CharUnicode(char *start) {
    unsigned char *bytes = (unsigned char*) start;
    int len = utf8len(bytes);
    static const unsigned int masks[5] = {0, 0x7F, 0x1F, 0x0F, 0x07};
    unsigned int unicode = bytes[0] & masks[len];
    for (int i = 1; i < len; i++) {
        unicode = (unicode << 6) | (bytes[i] & 0x3F);
    }
    return unicode;
}
String* mergeString(char *a, char *b) {
    StringBuilder *sb = newStringBuilder();
    appendMStringBuilder(sb, 2, a, b);
    String *s = newString(sb->string);
    freeStringBuilder(sb);
    return s;
}
CharIterator newCharIterator(const char *string) {
    return (CharIterator) {string, 0};
}

int hasNextChar(CharIterator *iterator) {
    return iterator->source[iterator->point] != '\0';
}

int asUtf8Char(const char* character) {
    int len = utf8len(character);
    if(len <= 0) throwTextsException("wrong UTF-8 character");
    int result = 0;
    char *asCharArray = (char*) &result;
    for (int i = 0; i < len; i++) {
        asCharArray[i] = character[i];
    }
    return result;
}

int nextChar(CharIterator *iterator) {
    int len = utf8len(iterator->source + iterator->point);
    if(len <= 0) throwTextsException("wrong UTF-8 character");
    int startPoint = iterator->point;
    iterator->point += len;
    int result = 0;
    char *asCharArray = (char*) &result; // 비트마스킹 쓰면 코드가 복잡해져서 result 변수의 주소를 charArray로 속이는 트릭 사용
    for (int i = 0; i < len; i++) {
        asCharArray[i] = iterator->source[startPoint+i];
    }
    return result;
}

/**
 * StringBuilder
 */
StringBuilder* newStringBuilder() {
    StringBuilder *sb = malloc(sizeof(StringBuilder));
    sb->size = 0;
    sb->cap = 100;
    sb->string = malloc(sizeof(char) * sb->cap);
    sb->string[0] = '\0';
    return sb;
}
void appendMStringBuilder(StringBuilder *sb, int num, ...) {
    va_list ap;
    va_start(ap, num);
    for (int i = 0; i < num; i++) {
        char *str = va_arg(ap, char*);
        appendStringBuilder(sb, str);
    }
    va_end(ap);
}
void freeStringBuilder(StringBuilder *sb) {
    free(sb->string);
    free(sb);
}
void checkAndExpand(StringBuilder *sb) {
    if(sb->size + 8 >= sb->cap) {
        sb->cap *= 2;
        sb->string = realloc(sb->string, sizeof(char) * sb->cap);
    }
}
void fillNullChar(StringBuilder *sb) {
    sb->string[sb->size] = '\0';
}
void appendCharStringBuilder(StringBuilder * sb, char c) {
    checkAndExpand(sb);
    sb->string[sb->size++] = c;
    fillNullChar(sb);
}
void appendUtf8StringBuilder(StringBuilder* sb, int utf8char) {
    char* asCharArray = (char*) &utf8char;
    int len = utf8len((const char*) &utf8char);
    //printf("LEN: %d\n", len);
    for(int i = 0; i<len; i++) {
        appendCharStringBuilder(sb, asCharArray[i]);
    }
}
void appendlnStringBuilder(StringBuilder* sb) {
    checkAndExpand(sb);
    sb->string[sb->size++] = '\n';
    fillNullChar(sb);
}
void appendStringBuilder(StringBuilder* sb, char *string) {
    int len = strlen(string);
    for(int i = 0; i<len; i++) {
        appendCharStringBuilder(sb, string[i]);
    }
}
void appendSpaceStringBuilder(StringBuilder *sb) {
    for(int i = 0; i<115; i++) appendlnStringBuilder(sb);
}
void appendIntStringBuilder(StringBuilder *sb, int value) {
    if(value == 0) {
        appendCharStringBuilder(sb, '0');
        return;
    }
    if(value < 0) {
        value *= -1;
        appendCharStringBuilder(sb, '-');
    }
    ArrayList *digits = newArrayList(10);
    while(value) {
        pushArrayList(digits, (void*) (value%10));
        value /= 10;
    }
    for(int i = digits->size - 1; i > -1; i--) {
        appendCharStringBuilder(sb, '0'+((int)digits->arr[i]));
    }
    freeArrayList(digits);
}


/**
 * WCharIterator
 */
#define IS_SURROGATE(WC)    (0xD800 <= (WC) && (WC) <= 0xDBFF)

WCharIterator newWCharIterator(wchar_t *string) {
    WCharIterator it;
    it.string = string;
    it.point = 0;
    return it;
}

LChar surrogateUtf16ToUtf32(wchar_t *wstr) {
    wchar_t first = *wstr;
    wchar_t second = wstr[1];
    return 0x10000 + (((first - 0xD800) << 10) | (second - 0xDC00));
}

LChar utf16ToUtf32(wchar_t *wstr) {
    wchar_t first = *wstr;
    if(IS_SURROGATE(first)) {
        wchar_t second = wstr[1];
        return 0x10000 + (((first - 0xD800) << 10) | (second - 0xDC00));
    }
    return (LChar) *wstr;
}

int hasNextLChar(WCharIterator *iterator) {
    return iterator->string[iterator->point] != L'\0';
}

LChar nextLChar(WCharIterator *iterator) {
    int inc = 1 + IS_SURROGATE(iterator->string[iterator->point]);
    wchar_t *next = iterator->string + iterator->point;
    iterator->point += inc;
    if(inc == 1) {
        return (LChar) *next;
    }
    return surrogateUtf16ToUtf32(next);
}