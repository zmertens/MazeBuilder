#include "sign.h"

#include <string>

#include <SDL3/SDL.h>

void sign_list_alloc(SignList *list, const std::size_t capacity) {
    list->capacity = capacity;
    list->size = 0;
    list->data = static_cast<Sign*>(SDL_calloc(capacity, sizeof(Sign)));
}

void sign_list_free(const SignList *list) {
    SDL_free(list->data);
}

void sign_list_grow(SignList *list) {
    SignList new_list;
    sign_list_alloc(&new_list, list->capacity * 2);
    SDL_memcpy(new_list.data, list->data, list->size * sizeof(Sign));
    SDL_free(list->data);
    list->capacity = new_list.capacity;
    list->data = new_list.data;
}

void _sign_list_add(SignList *list, const Sign *sign) {
    if (list->size == list->capacity) {
        sign_list_grow(list);
    }
    Sign *e = list->data + list->size++;
    memcpy(e, sign, sizeof(Sign));
}

void sign_list_add(
    SignList *list, const int x, const int y, const int z, const int face, const std::string_view text)
{
    sign_list_remove(list, x, y, z, face);
    Sign sign;
    sign.x = x;
    sign.y = y;
    sign.z = z;
    sign.face = face;
    SDL_strlcpy(sign.text, text.data(), MAX_SIGN_LENGTH);
    sign.text[MAX_SIGN_LENGTH - 1] = '\0';
    _sign_list_add(list, &sign);
}

int sign_list_remove(SignList *list, const int x, const int y, const int z, const int face) {
    int result = 0;
    for (int i = 0; i < list->size; i++) {
        if (Sign *e = list->data + i; e->x == x && e->y == y && e->z == z && e->face == face) {
            const Sign *other = list->data + (--list->size);
            SDL_memcpy(e, other, sizeof(Sign));
            i--;
            result++;
        }
    }
    return result;
}

int sign_list_remove_all(SignList *list, const int x, const int y, const int z) {
    int result = 0;
    for (int i = 0; i < list->size; i++) {
        if (Sign *e = list->data + i; e->x == x && e->y == y && e->z == z) {
            const Sign *other = list->data + --list->size;
            SDL_memcpy(e, other, sizeof(Sign));
            i--;
            result++;
        }
    }
    return result;
}
