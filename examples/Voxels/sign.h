#ifndef SING_H
#define SING_H

#include <string_view>

#define MAX_SIGN_LENGTH 16

struct Sign {
    int x;
    int y;
    int z;
    int face;
    char text[MAX_SIGN_LENGTH];
};

struct SignList {
    std::size_t capacity;
    std::size_t size;
    Sign *data;
};

void sign_list_alloc(SignList *list, std::size_t capacity);
void sign_list_free(const SignList *list);
void sign_list_grow(SignList *list);
void sign_list_add(
    SignList *list, int x, int y, int z, int face, std::string_view text);
int sign_list_remove(SignList *list, int x, int y, int z, int face);
int sign_list_remove_all(SignList *list, int x, int y, int z);

#endif // SING_H
