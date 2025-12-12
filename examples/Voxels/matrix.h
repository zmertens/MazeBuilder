#ifndef MATRIX_H
#define MATRIX_H

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEGREES(radians) ((radians) * 180.0 / M_PI)
#define RADIANS(degrees) ((degrees) * M_PI / 180.0)
#define SIGN(x) (((x) > 0) - ((x) < 0))

void normalize(float *x, float *y, float *z);
void mat_identity(float *matrix);
void mat_translate(float *matrix, float dx, float dy, float dz);
void mat_rotate(float *matrix, float x, float y, float z, float angle);
void mat_vec_multiply(float *vector, const float *a, const float *b);
void mat_multiply(float *matrix, const float *a, const float *b);
void mat_apply(float *data, const float *matrix, int count, int offset, int stride);
void frustum_planes(float planes[6][4], int radius, const float *matrix);
void mat_frustum(
    float *matrix, float left, float right, float bottom,
    float top, float znear, float zfar);
void mat_perspective(
    float *matrix, float fov, float aspect,
    float near, float far);
void mat_ortho(
    float *matrix,
    float left, float right, float bottom, float top, float near, float far);
void set_matrix_2d(float *matrix, int width, int height);
void set_matrix_3d(
    float *matrix, int width, int height,
    float x, float y, float z, float rx, float ry,
    float fov, int ortho, int radius);
void set_matrix_item(float *matrix, int width, int height, int scale);

void compute_sight_vector(float rx, float ry, float& vx, float& vy, float& vz) noexcept;
void compute_motion_vector(const int flying, const int sz, const int sx, const float rx, const float ry,
                                  float* vx, float* vy, float* vz) noexcept;

#endif
