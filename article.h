#ifndef ARITCLE_H
#define ARITCLE_H

#include "util/texts.h"
#include "util/serialization.h"
#include "db.h"

typedef enum {
    ECATEGORY_ACTION,
    ECATEGORY_COMEDY,
    ECATEGORY_SF,
    ECATEGORY_MUSIC,
    ECATEGORY_WAR,
    ECATEGORY_NOIR,
    ECATEGORY_ROMANCE,
    ECATEGORY_ETC
} Category;

#define CATEGORY_LAST   (ECATEGORY_ETC)
#define CATEGORY_SIZE   (8)

typedef enum {
    ERATING_ALL,
    ERATING_12,
    ERATING_15,
    ERATING_19
} Rating;

#define RATING_LAST (ERATING_19)
#define RATING_SIZE (4)

typedef struct {
    int id;
    String *name;
    String *description;
    Category category;
    Rating rating;
} Article;

Article* newArticle();
Article* cloneArticle(Article *article);
void freeArticle(Article *article);
extern InstanceAccessor ARTICLE_ACCESSOR;
extern Serializer ARTICLE_SERIALIZER;
extern Serializer ARTICLE_DB_SERIALIZER;
DB* newArticleDB();

#define ARTICLE_CACHE_NAME  (0) 
#define ARTICLE_CACHE_CATEGORY  (1) 
#define ARTICLE_SEARCH_NAME  (0) 

extern char CATEGORY_NAMES[][16];
extern char RATING_NAMES[][16];

#endif