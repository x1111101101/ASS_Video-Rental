#ifndef MATH_H
#define MATH_H

#define MAX(X, Y)   ((X) > (Y)) ? (X) : (Y)
#define MIN(X, Y)   ((X) < (Y)) ? (X) : (Y)

typedef struct {
    int min, max;
} IntRange;

int fitRange(int value, int min, int max);

#endif