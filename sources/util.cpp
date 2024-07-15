/**
 * Utility functions for handling opengl operations, RNG, and meshes
 *
*/

#include "util.h"

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include <fstream>

#include <SDL3/SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

int rand_int(int n) {
    int result;
    while (n <= (result = rand() / (RAND_MAX / n)));
    return result;
}

double rand_double() {
    return (double)rand() / (double)RAND_MAX;
}

void update_fps(FPS *fps) {
    fps->frames++;
    double now = SDL_GetTicks();
    double elapsed = now - fps->since;
    if (elapsed >= 1) {
         fps->fps = round(fps->frames / elapsed);
        fps->fps = SDL_roundf(fps->frames * 1000.0 / elapsed);
        fps->frames = 0;
        fps->since = now;
    }
}

char *load_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "fopen %s failed: %d %s\n", path, errno, strerror(errno));
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    rewind(file);
    char *data = (char*) calloc(length + 1, sizeof(char));
    fread(data, 1, length, file);
    fclose(file);
    return data;
}

/**
 * @brief Private function to load file using SDL-specific functions
 */
char* load_file_using_sdl(const char* path) {

#if defined(MAZE_DEBUG)
    SDL_Log("Reading file %s\n", path);
#endif

    // Open binary file
    SDL_IOStream* io = SDL_IOFromFile(path, "r");
    if (io == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_IOFromFile failed: %s", SDL_GetError());
        return nullptr;
    }
    auto data_size = SDL_GetIOSize(io);
    // Allocate memory for the file content + null terminator
    auto data = (char*) SDL_malloc(data_size + 1);

    if (data == nullptr) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_malloc failed: %s", SDL_GetError());
        SDL_CloseIO(io);
		return nullptr;
	}

    // Read file into memory
    auto nb_read_total = 0, nb_read = SDL_TRUE;
    auto buf = data;
    while (nb_read_total < data_size && nb_read != SDL_FALSE) {
        nb_read = SDL_ReadIO(io, buf, (data_size - nb_read_total));
        nb_read_total += nb_read;
        buf += nb_read;
    }

    SDL_CloseIO(io);
    if (nb_read_total != data_size) {
        SDL_free(data);
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to read complete file: %s", SDL_GetError());
        return nullptr;
    }
    data[nb_read_total] = '\0';
    return data;
}

GLuint make_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        GLchar *info = (GLchar*) calloc(length, sizeof(GLchar));
        glGetShaderInfoLog(shader, length, NULL, info);
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "glCompileShader failed:\n%s\n", info);
        SDL_free(info);
    }
    return shader;
}

GLuint load_shader(GLenum type, const char *path) {
    char *data = load_file_using_sdl(path);
    GLuint result = make_shader(type, data);
    SDL_free(data);
    return result;
}

GLuint make_program(GLuint shader1, GLuint shader2) {
    GLuint program = glCreateProgram();
    glAttachShader(program, shader1);
    glAttachShader(program, shader2);
    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        GLchar *info = (GLchar*) calloc(length, sizeof(GLchar));
        glGetProgramInfoLog(program, length, NULL, info);
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "glLinkProgram failed: %s\n", info);
        SDL_free(info);
    }
    glDetachShader(program, shader1);
    glDetachShader(program, shader2);
    glDeleteShader(shader1);
    glDeleteShader(shader2);
    return program;
}

GLuint load_program(const char *path1, const char *path2) {
    GLuint shader1 = load_shader(GL_VERTEX_SHADER, path1);
    GLuint shader2 = load_shader(GL_FRAGMENT_SHADER, path2);
    GLuint program = make_program(shader1, shader2);
    return program;
}

void flip_image_vertical(
    unsigned char *data, unsigned int width, unsigned int height)
{
    unsigned int size = width * height * 4;
    unsigned int stride = sizeof(char) * width * 4;
    unsigned char *new_data = (unsigned char*) malloc(sizeof(unsigned char) * size);
    for (unsigned int i = 0; i < height; i++) {
        unsigned int j = height - i - 1;
        SDL_memcpy(new_data + j * stride, data + i * stride, stride);
    }
    SDL_memcpy(data, new_data, size);
    SDL_free(new_data);
}

void load_png_texture(const char *file_name) {
    unsigned int error;
    int width, height;
    // n stores bits-per-pixel
    int n;
    unsigned char *data = stbi_load(file_name, &width, &height, &n, 0);
    // ... replace '0' with '1'..'4' to force that many components per pixel
    if (data == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "std_load %s failed, error %u\n", file_name, error);
    }
    flip_image_vertical(data, width, height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
}

char *tokenize(char *str, const char *delim, char **key) {
    char *result;
    if (str == NULL) {
        str = *key;
    }
    str += strspn(str, delim);
    if (*str == '\0') {
        return NULL;
    }
    result = str;
    str += strcspn(str, delim);
    if (*str) {
        *str++ = '\0';
    }
    *key = str;
    return result;
}

int char_width(char input) {
    static constexpr int lookup[128] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        4, 2, 4, 7, 6, 9, 7, 2, 3, 3, 4, 6, 3, 5, 2, 7,
        6, 3, 6, 6, 6, 6, 6, 6, 6, 6, 2, 3, 5, 6, 5, 7,
        8, 6, 6, 6, 6, 6, 6, 6, 6, 4, 6, 6, 5, 8, 8, 6,
        6, 7, 6, 6, 6, 6, 8,10, 8, 6, 6, 3, 6, 3, 6, 6,
        4, 7, 6, 6, 6, 6, 5, 6, 6, 2, 5, 5, 2, 9, 6, 6,
        6, 6, 6, 6, 5, 6, 6, 6, 6, 6, 6, 4, 2, 5, 7, 0
    };
    return lookup[input];
}

int string_width(const char *input) {
    int result = 0;
    int length = SDL_strlen(input);
    for (int i = 0; i < length; i++) {
        result += char_width(input[i]);
    }
    return result;
}

int wrap(const char *input, int max_width, char *output, int max_length) {
    *output = '\0';
    char *text = (char*) SDL_malloc(sizeof(char) * (SDL_strlen(input) + 1));
    strcpy(text, input);
    int space_width = char_width(' ');
    int line_number = 0;
    char *key1, *key2;
    char *line = tokenize(text, "\r\n", &key1);
    while (line) {
        int line_width = 0;
        char *token = tokenize(line, " ", &key2);
        while (token) {
            int token_width = string_width(token);
            if (line_width) {
                if (line_width + token_width > max_width) {
                    line_width = 0;
                    line_number++;
                    strncat(output, "\n", max_length - strlen(output) - 1);
                }
                else {
                    strncat(output, " ", max_length - strlen(output) - 1);
                }
            }
            strncat(output, token, max_length - strlen(output) - 1);
            line_width += token_width + space_width;
            token = tokenize(NULL, " ", &key2);
        }
        line_number++;
        strncat(output, "\n", max_length - strlen(output) - 1);
        line = tokenize(NULL, "\r\n", &key1);
    }
    SDL_free(text);
    return line_number;
}
