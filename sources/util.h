#ifndef _util_h_
#define _util_h_

#include <glad/glad.h>

#include <string>
#include <vector>

#define PI 3.14159265359
#define DEGREES(radians) ((radians) * 180 / PI)
#define RADIANS(degrees) ((degrees) * PI / 180)
#define ABS(x) ((x) < 0 ? (-(x)) : (x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SIGN(x) (((x) > 0) - ((x) < 0))

typedef struct {
    unsigned int fps;
    unsigned int frames;
    double since;
} FPS;

int rand_int(int n);
double rand_double();
void update_fps(FPS *fps);

GLfloat *malloc_faces(int components, int faces);
GLuint gen_faces(int components, int faces, GLfloat *data);
GLuint make_shader(GLenum type, const char *source);
GLuint load_shader(GLenum type, const char *path);
GLuint make_program(GLuint shader1, GLuint shader2);
GLuint load_program(const char *path1, const char *path2);
void load_png_texture(const char *file_name);
char *tokenize(char *str, const char *delim, char **key);
int char_width(char input);
int string_width(const char *input);
int wrap(const char *input, int max_width, char *output, int max_length);
void dump_opengl_info(bool dumpExtensions);

std::string convert_data_to_str(int faces, GLfloat *data);

#define gl_check_for_error() glCheckError_(__FILE__, __LINE__)
GLenum glCheckError_(const char *file, int line);

#endif
