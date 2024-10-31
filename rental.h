#ifndef RENTAL_H
#define RENTAL_H

#include "db.h"
#include "util/serialization.h"

typedef struct {
    int id;
    int userId;
    int videoId;
    int rentTime, returnTime;
} Rental;

Rental* newRental();
void freeRental(Rental *rental);
DB* newRentalDB();
int isReturnedRental(Rental *rental);

#define RENTAL_CACHE_USERID     (0)
#define RENTAL_CACHE_VIDEOID    (1)

extern InstanceAccessor RENTAL_ACCESSOR;
extern Serializer RENTAL_SERIALIZER;
extern Serializer RENTAL_DB_SERIALIZER;
extern Comparator *RENTAL_ID_COMPARATOR;

#endif