/**
 * Utility functions for handling OpenGL operations
 *
*/

#include "craft_utils.h"

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

using namespace std;

GLenum _check_for_gl_err(const char* file, int line) noexcept {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error = "";
        switch (errorCode) {
        case GL_INVALID_ENUM: error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE: error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: error = "INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY: error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,
            "OpenGL ERROR: %s\n\t\tFILE: %s, LINE: %d\n", error.c_str(), file, line);
    }
    return errorCode;
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
    // SDL_ReadIO returns the number of bytes read, or 0 on error or end of file
    int nb_read_total = 0, nb_read_size = 1;
    auto buf = data;
    while (nb_read_total < data_size && nb_read_size != 0) {
        nb_read_size = SDL_ReadIO(io, buf, (data_size - nb_read_total));
        nb_read_total += nb_read_size;
        buf += nb_read_size;
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
        GLchar *info = (GLchar*) SDL_calloc(length, sizeof(GLchar));
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
    unsigned char *new_data = (unsigned char*) SDL_malloc(sizeof(unsigned char) * size);
    for (unsigned int i = 0; i < height; i++) {
        unsigned int j = height - i - 1;
        if (data != 0 && new_data != 0) {
            SDL_memcpy(new_data + j * stride, data + i * stride, stride);
        }
    }
    if (data != 0 && new_data != 0) {
        SDL_memcpy(data, new_data, size);
        SDL_free(new_data);
    }
}

void load_png_texture(const char *file_name) {
    int width, height;
    // n stores bits-per-pixel
    int n;
    unsigned char *data = stbi_load(file_name, &width, &height, &n, 0);
    // ... replace '0' with '1'..'4' to force that many components per pixel
    if (data == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "std_load %s failed!!\n", file_name);
    }
    flip_image_vertical(data, width, height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
}

unsigned int load_cubemap(const vector<string>& files) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (auto i = 0; i < files.size(); i++) {
        auto *data = stbi_load(files.at(i).c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            stbi_set_flip_vertically_on_load(false);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Cubemap tex failed to load at path: %s\n", files.at(i).c_str());
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
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
