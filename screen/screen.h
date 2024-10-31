#ifndef SCREEN_H
#define SCREEN_H

#include "../service.h"

void mainScreen(Service *service);

// user
void userMainScreen(Service *service);
void userArticleListByCategoryScreen(Service *service, User *user, Category category);
void userArticleListScreen(Service *service, User *user);
void userVideoListByArticleScreen(Service *service, User *user, Article *article);
void userInfoScreen(Service *service, User *user);

// admin
void adminMainScreen(Service *service);
void articleManageScreen(Service *service);
void userManageScreen(Service *service);
void userEditScreen(Service *service, User *base);


#define RENTAL_ACTION_EXIT  (0)
#define RENTAL_ACTION_QUERY  (1)
#define RENTAL_ACTION_REFRESH  (2)
void rentalListScreen(Service *service, User *user);
void returnScreen(Service *service, User *user);
int rentalAdminScreen(Service *service, ArrayList *list);

#endif