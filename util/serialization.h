#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <stdio.h>
#include "texts.h"
#include "date.h"

typedef struct {
    void (*serialize)(FILE *file, void *instance);
    void* (*deserialize)(FILE *file);
} Serializer;

void serializeString(FILE *file, String *value);
String* deserializeString(FILE *file);

void serializeInt(FILE *file, int value);
int deserializeInt(FILE *file);

void serializeDate(FILE *file, Date date);
Date deserializeDate(FILE *file);


#endif