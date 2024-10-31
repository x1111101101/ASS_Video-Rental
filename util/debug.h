#ifndef MYDEBUG_H
#define MYDEBUG_H

#ifdef debugenabled
#define df(X)   printf(X)
#define df1(X, a1)   printf(X, a1)
#define df2(X, a1, a2)   printf(X, a1, a2)
#define df3(X, a1, a2, a3)   printf(X, a1, a2, a3)

#else

#define df(X)   debug_print_nothing()
#define df1(X, a1)   debug_print_nothing()
#define df2(X, a1, a2)   debug_print_nothing()
#define df3(X, a1, a2, a3)   debug_print_nothing()
#endif

void d(char *msg);
void debug_print_nothing();

#endif