#ifndef USER_H
#define USER_H
#include "util/texts.h"
#include "util/mystructures.h"
#include "util/serialization.h"
#include "util/date.h"
#include "db.h"

#define USER_NAME_LEN   (48)
#define USER_PHONE_LEN   (24)

typedef struct {
    int id;
    String *name;
    String *phone;
    String *loginId;
    Date birthday;
    int passwordHash;
} User;

extern InstanceAccessor USER_ACCESSOR;
extern Serializer USER_SERIALIZER;
extern Serializer USER_DB_SERIALIZER;

User* newUser();
void freeUser(User* user);
DB* newUserDB();

#define USER_CACHE_LOGINID  (0)
#define USER_CACHE_PHONE    (1)

#define USER_SEARCH_NAME    (0)
#define USER_SEARCH_PHONE   (1)

#endif