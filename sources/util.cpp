/**
 * Utility functions for handling opengl operations, RNG, and meshes
 *
*/

#include "util.h"

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>

#include <SDL3/SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <thinks/obj_io/obj_io.h>
#include <thinks/obj_io/mesh_types.h>
#include <thinks/obj_io/read_write_utils.h>

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
        // fps->fps = round(fps->frames / elapsed);
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

/*
Private function to load file using SDL-specific functions
*/
//char* load_file_using_sdl(const char* path) {
//    
//    char* data;
//
//    // Open binary file
//    SDL_RWops* io = SDL_RWFromFile(path, "rb");
//    if (io != nullptr) {
//        Sint64 data_size = SDL_RWsize(io);
//        data = (char*) SDL_malloc(data_size + 1);
//        if (SDL_RWread(io, data, data_size) != data_size) {
//            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_RWread failed: %s\n", SDL_GetError());
//        }
//#if defined(DEBUGGING)
//        SDL_Log("Reading file % s\n", path);
//#endif
//        SDL_RWclose(io);
//    } else {
//        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ERROR: SDL_RWops failed: %s", SDL_GetError());
//    }
//
//    return data;
//}

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
        fprintf(stderr, "glCompileShader failed:\n%s\n", info);
        free(info);
    }
    return shader;
}

GLuint load_shader(GLenum type, const char *path) {
    char *data = load_file(path);
    GLuint result = make_shader(type, data);
    free(data);
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
        fprintf(stderr, "glLinkProgram failed: %s\n", info);
        free(info);
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
    free(new_data);
}

void load_png_texture(const char *file_name) {
    unsigned int error;
    int width, height;
    // n stores bits-per-pixel
    int n;
    unsigned char *data = stbi_load(file_name, &width, &height, &n, 0);
    // ... replace '0' with '1'..'4' to force that many components per pixel
    if (data == nullptr) {
        fprintf(stderr, "stb_load %s failed, error %u\n", file_name, error);
        exit(1);
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
    static const int lookup[128] = {
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
    int length = strlen(input);
    for (int i = 0; i < length; i++) {
        result += char_width(input[i]);
    }
    return result;
}

int wrap(const char *input, int max_width, char *output, int max_length) {
    *output = '\0';
    char *text = (char*) malloc(sizeof(char) * (strlen(input) + 1));
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
    free(text);
    return line_number;
}

void dump_opengl_info(bool dumpExtensions) {
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *version = glGetString(GL_VERSION);
    const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
	
	SDL_Log("-------------------------------------------------------------\n");
    SDL_Log("GL Vendor    : %s\n", vendor);
    SDL_Log("GL Renderer  : %s\n", renderer);
    SDL_Log("GL Version   : %s\n", version);
    SDL_Log("GL Version   : %d.%d\n", major, minor);
    SDL_Log("GLSL Version : %s\n", glslVersion);
    SDL_Log("-------------------------------------------------------------\n");

    if (dumpExtensions) {
        GLint nExtensions;
        glGetIntegerv(GL_NUM_EXTENSIONS, &nExtensions);
        for (int i = 0; i < nExtensions; i++) {
            SDL_Log("%s\n", glGetStringi(GL_EXTENSIONS, i));
        }
    }
}

/**
 * @brief convert_grid_to_str take the grid vertex data and transform into C++ string data
 * From the data, a task writer in Craft will call C++ fstream functions with the string data
 * @param faces
 * @param data
 * @return
 */
std::string convert_grid_to_str(int faces, GLfloat *data) {
    using namespace std;

#if defined(DEBUGGING)
    SDL_Log("faces: %d, data: %p\n", faces, data);
#endif

    using MeshType = TriangleMesh<>;
    using IndexType = MeshType::IndexType;
    using VertexType = MeshType::VertexType;
    using PositionType = VertexType::PositionType;
    using TexCoordType = VertexType::TexCoordType;
    using NormalType = VertexType::NormalType;

    auto my_mesh = MeshType{};
    // for each visible cube face or plant face
    for (int i = 0; i < faces; i++) {
        // each face has 6 points (2 triangles, 3 vertices each)
        my_mesh.indices.emplace_back(static_cast<std::uint16_t>(faces - i));
        for (int j = 0; j < 6; j++) {
            // each vertex has 10 values (x, y, z, nx, ny, nz, u, v, ao, light)
            int k = i * 60 + j * 10;
            GLfloat x = data[k+0];
            GLfloat y = data[k+1];
            GLfloat z = data[k+2];
            GLfloat nx = data[k+3];
            GLfloat ny = data[k+4];
            GLfloat nz = data[k+5];
            GLfloat u = data[k+6];
            GLfloat v = data[k+7];
            my_mesh.vertices.emplace_back(VertexType{PositionType{x, y, z}, TexCoordType{u, v}, NormalType{nx, ny, nz}});
        }
    }

    struct WriteResult {
        thinks::ObjWriteResult write_result;
        std::string mesh_str;
    };

    // write normals and tex coords too
    auto&& mesh_result = WriteMesh(my_mesh, true, true);


    return mesh_result.mesh_str;
}

GLenum glCheckError_(const char *file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error = "";
        switch (errorCode)
        {
            case GL_INVALID_ENUM: error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE: error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: error = "INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW: error = "STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW: error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY: error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, 
            "OpenGL ERROR: %s\n\t\tFILE: %s, LINE: %d\n", error.c_str(), file, line);
    }
    return errorCode;
}

