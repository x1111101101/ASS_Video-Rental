#include "date.h"
#include <time.h> 

static int isLeap(int year) {
    return !(year%100 == 0 && year%400 != 0) && year%4 == 0;
}

Date plusDayDate(Date *date, int days) {

}

int toIntDate(Date *date) {
    int days = 0;
    for(int i = DATE_MIN_YEAR; i<date->year; i++) {
        days += 365 + isLeap(i);
    }
    int daysPerMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if(isLeap(date->year)) daysPerMonth[1]++;
    for(int m = 1; m<date->month; m++) {
        days += daysPerMonth[m-1];
    }
    days += date->day - 1;
    return days;
}
Date fromIntDate(int value) {
    Date date = {DATE_MIN_YEAR, 1, 1};
    while((isLeap(date.year) && value >= 366) || value >= 365) {
        int daysPerMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if(isLeap(date.year)) {
            daysPerMonth[1]++;
            value -= 366;
        } else value -= 365;
        date.year++;
    }
    int daysPerMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if(isLeap(date.year)) daysPerMonth[1]++;
    while(value >= daysPerMonth[date.month-1]) value -= daysPerMonth[-1 + date.month++];
    date.day = value+1;
    return date;
}

Date currentDate() {
    time_t t=time(NULL);
  	struct tm tm = *localtime(&t);
    Date date = {tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday};
    return date;
}

int isValidDate(Date *date) {
    if(date->year < DATE_MIN_YEAR) return 0;
    if(date->month > 12 || date->month < 1) return 0;
    int daysPerMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if(isLeap(date->year)) {
        daysPerMonth[1]++;
    }
    if(date->day < 1 || date->day > daysPerMonth[date->month - 1]) return 0;
    return 1;
}

int isPastDate(Date *pivot, Date *compare) {
    if(pivot->year < compare->year) return 1;
    if(pivot->year > compare->year) return 0;
    if(pivot->month < compare->month) return 1;
    if(pivot->month > compare->month) return 0;
    return pivot->day < compare->day;
}

String* toStringDate(Date *date) {
    StringBuilder *sb = newStringBuilder();
    appendIntStringBuilder(sb, date->year);
    appendStringBuilder(sb, "년 ");
    appendIntStringBuilder(sb, date->month);
    appendStringBuilder(sb, "월 ");
    appendIntStringBuilder(sb, date->day);
    appendStringBuilder(sb, "일");
    String *result = newString(sb->string);
    freeStringBuilder(sb);
    return result;
}

/**
 * DateTime
 */
int secondOfDateTime(DateTime *dateTime) {
    return dateTime->time%60;
}
int minuteOfDateTime(DateTime *dateTime) {
    return (dateTime->time%3600)/60;
}
int hourOfDateTime(DateTime *dateTime) {
    return dateTime->time/3600;
}