#ifndef _sign_h_
#define _sign_h_

#include <cstdlib>

#define MAX_SIGN_LENGTH 16

typedef struct {
    int x;
    int y;
    int z;
    int face;
    char text[MAX_SIGN_LENGTH];
} Sign;

typedef struct {
    std::size_t capacity;
    std::size_t size;
    Sign *data;
} SignList;

void sign_list_alloc(SignList *list, std::size_t capacity);
void sign_list_free(SignList *list);
void sign_list_grow(SignList *list);
void sign_list_add(
    SignList *list, int x, int y, int z, int face, const char *text);
int sign_list_remove(SignList *list, int x, int y, int z, int face);
int sign_list_remove_all(SignList *list, int x, int y, int z);

#endif
