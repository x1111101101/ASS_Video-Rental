#ifndef SERVICE_H
#define SERVICE_H

#include "db.h"
#include "user.h"
#include "article.h"
#include "video.h"
#include "rental.h"
#include "util/date.h"

typedef struct {
    User *loginUser;
    DB *users;
    DB *videos;
    DB *rentals;
    DB *articles;
} Service;

typedef struct {
    int succeed;
    String *msg;
} Response;

typedef struct {
    String *loginId, *password, *name, *phone;
    Date birthday;
} SignupForm;

typedef struct {
    int total, left;
} VideoStock;

Service* newService();
Service* loadService();
void initService(Service *service);

// Rent, Return
void returnVideoService(Service *service, Rental *rental);
Response rentVideoService(Service *service, int userId, int videoId);

// User Account
int tryLoginService(Service *service, String *loginId, String *password);
Response signupService(Service *service, SignupForm form);
int isLoginedService(Service *service);
User* getLoginService(Service *service);
void logoutService(Service *service);
// User CRUDQ
ArrayList* getAllUsersService(Service *service);
ArrayList* queryUserService(Service *service, String *query);
Response editUserService(Service *service, int id, SignupForm form);
User* getUserByIdService(Service *service, int id);
Response removeUserService(Service *service, int id);
void changeUserPasswordService(Service *service, User *user, String *password);

// Article CRUDQ
ArrayList* getAllArticlesService(Service *service);
Article* getArticleByIdService(Service *service, int id);
ArrayList* getArticlesByCategoryService(Service *service, Category category);
Response createArticleService(Service *service, Article *article);
Response editArticleService(Service *service, int id, Article *edited);
ArrayList* queryArticlesService(Service *service, String *query);
void removeArticleService(Service *service, Article *article);
int checkArticleAgeService(Service *service, User *user, Article *article);
VideoStock getVideoStockService(Service *service, int articleId);
int isArticleRentedByUserService(Service *service, int articleId, int userId);
Rental* getCurrentRentalByArticleAndUser(Service *service, int articleId, int userId);

// Video CRUDQ
Response createVideoService(Service *service, Article *article, String *videoId);
Video* getVideoByVideoIdService(Service *service, String *videoId);
Video* getVideoByIdService(Service *service, int id);
ArrayList* getVideosService(Service *service, int articleId);
ArrayList* queryVideosService(Service *service, String *query);
void removeVideoService(Service *service, Video *video);

// Rental CRUDQ
ArrayList* getRentalsByArticleService(Service *service, int articleId);
ArrayList* getRentalsByVideoService(Service *service, int videoId);
ArrayList* getRentalsByUserService(Service *service, int userId);
void removeRentalService(Service *service, Rental *rental);
Rental* getRentalByIdService(Service *service, int id);
ArrayList* getAllRentalsService(Service *service);

#endif