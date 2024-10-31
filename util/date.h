#ifndef DATE_H
#define DATE_H

#include "texts.h"

#define DATE_MIN_YEAR   (1800)

typedef struct {
    int year;
    char month, day;
} Date;

int isPastDate(Date *pivot, Date *compare);
int isValidDate(Date *date);
/**
 * 1800년 1월 1일과의 차이
 */
int toIntDate(Date *date);
Date fromIntDate(int value);
Date currentDate();
String* toStringDate(Date *date);
Date plusDayDate(Date *date, int days);

typedef struct {
    Date date;
    int time;
} DateTime;

int secondOfDateTime(DateTime *dateTime);
int minuteOfDateTime(DateTime *dateTime);
int hourOfDateTime(DateTime *dateTime);

#endif