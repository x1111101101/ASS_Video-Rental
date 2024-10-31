#include "serialization.h"
#include <stdio.h>
#include <stdlib.h>

static void _throw(char *msg) {
    printf(msg);
    getchar();
    exit(1);
}

void serializeString(FILE *file, String *value) {
    int len = value->len;
    serializeInt(file, len);
    fwrite(value->content, sizeof(char), len, file);
}

String* deserializeString(FILE *file) {
    int len = deserializeInt(file);
    char *buf = malloc(sizeof(char) * (len + 1));
    fread(buf, sizeof(char), len, file);
    buf[len] = '\0';
    String *value = newString(buf);
    free(buf);
    return value;
}

void serializeInt(FILE *file, int value) {
    fwrite(&value, sizeof(int), 1, file);
}

int deserializeInt(FILE *file) {
    int buf;
    fread(&buf, sizeof(int), 1, file);
    return buf;
}

void serializeDate(FILE *file, Date date) {
    serializeInt(file, date.year);
    serializeInt(file, date.month);
    serializeInt(file, date.day);
}
Date deserializeDate(FILE *file) {
    Date d;
    d.year = deserializeInt(file);
    d.month = deserializeInt(file);
    d.day = deserializeInt(file);
    return d;
}