#include "math.h"

int fitRange(int value, int min, int max) {
    return MAX(min, MIN(value, max));
}