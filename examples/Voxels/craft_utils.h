#ifndef CRAFT_UTILS_H
#define CRAFT_UTILS_H

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <string>
#include <vector>
#include <functional>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEGREES(radians) ((radians) * 180 / M_PI)
#define RADIANS(degrees) ((degrees) * M_PI / 180)
#define SIGN(x) (((x) > 0) - ((x) < 0))

GLenum _check_for_gl_err(const char* file, int line) noexcept;
#define CHECK_GL_ERR() _check_for_gl_err(__FILE__, __LINE__)

GLuint make_shader(GLenum type, const char *source);
GLuint load_shader(GLenum type, const char *path);
GLuint make_program(GLuint shader1, GLuint shader2);
GLuint load_program(const char *path1, const char *path2);
void load_png_texture(const char *file_name);
unsigned int load_cubemap(const std::vector<std::string>& files);
char *tokenize(char *str, const char *delim, char **key);
int char_width(char input);
int string_width(const char *input);
int wrap(const char *input, int max_width, char *output, int max_length);

#endif // CRAF_UTILS_H
