#ifndef VIDEO_H
#define VIDEO_H

#include "db.h"
#include "util/serialization.h"
#include "util/texts.h"
#include "util/date.h"
#include "util/mystructures.h"

#define VIDEO_CACHE_VIDEOID     (0)
#define VIDEO_CACHE_ARTICLEID   (1)
#define VIDEO_CACHE_RENTALID    (2)
#define VIDEO_SEARCH_VIDEOID    (0)

typedef struct {
    int id;
    String *videoId;
    int articleId;
    int rentalId;
    Date storedDate;
} Video;

Video* newVideo();
void freeVideo(Video *video);
DB* newVideoDB();
String* toStringVideo(Video *video);

extern InstanceAccessor VIDEO_ACCESSOR;
extern Serializer VIDEO_SERIALIZER;
extern Serializer VIDEO_DB_SERIALIZER;
extern Comparator *VIDEO_ID_COMPARATOR;

#endif