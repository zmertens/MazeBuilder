#include "matrix.h"

#include <SDL3/SDL.h>

void matrix::normalize(float *x, float *y, float *z) noexcept
{
    const float d = SDL_sqrtf(*x * *x + *y * *y + *z * *z);
    *x /= d;
    *y /= d;
    *z /= d;
}

void matrix::identity(float *matrix) noexcept
{
    matrix[0] = 1;
    matrix[1] = 0;
    matrix[2] = 0;
    matrix[3] = 0;
    matrix[4] = 0;
    matrix[5] = 1;
    matrix[6] = 0;
    matrix[7] = 0;
    matrix[8] = 0;
    matrix[9] = 0;
    matrix[10] = 1;
    matrix[11] = 0;
    matrix[12] = 0;
    matrix[13] = 0;
    matrix[14] = 0;
    matrix[15] = 1;
}

void matrix::translate(float *matrix, const float dx, const float dy, const float dz) noexcept
{
    matrix[0] = 1;
    matrix[1] = 0;
    matrix[2] = 0;
    matrix[3] = 0;
    matrix[4] = 0;
    matrix[5] = 1;
    matrix[6] = 0;
    matrix[7] = 0;
    matrix[8] = 0;
    matrix[9] = 0;
    matrix[10] = 1;
    matrix[11] = 0;
    matrix[12] = dx;
    matrix[13] = dy;
    matrix[14] = dz;
    matrix[15] = 1;
}

void matrix::rotate(float *matrix, float x, float y, float z, const float angle) noexcept
{
    normalize(&x, &y, &z);
    const float s = SDL_sinf(angle);
    const float c = SDL_cosf(angle);
    const float m = 1 - c;
    matrix[0] = m * x * x + c;
    matrix[1] = m * x * y - z * s;
    matrix[2] = m * z * x + y * s;
    matrix[3] = 0;
    matrix[4] = m * x * y + z * s;
    matrix[5] = m * y * y + c;
    matrix[6] = m * y * z - x * s;
    matrix[7] = 0;
    matrix[8] = m * z * x - y * s;
    matrix[9] = m * y * z + x * s;
    matrix[10] = m * z * z + c;
    matrix[11] = 0;
    matrix[12] = 0;
    matrix[13] = 0;
    matrix[14] = 0;
    matrix[15] = 1;
}

void matrix::vec_multiply(float *vector, const float *a, const float *b) noexcept
{
    float result[4];
    for (int i = 0; i < 4; i++)
    {
        float total = 0;
        for (int j = 0; j < 4; j++)
        {
            int p = j * 4 + i;
            int q = j;
            total += a[p] * b[q];
        }
        result[i] = total;
    }
    for (int i = 0; i < 4; i++)
    {
        vector[i] = result[i];
    }
}

void matrix::multiply(float *matrix, const float *a, const float *b) noexcept
{
    float result[16];
    for (int c = 0; c < 4; c++)
    {
        for (int r = 0; r < 4; r++)
        {
            const int index = c * 4 + r;
            float total = 0;
            for (int i = 0; i < 4; i++)
            {
                const int p = i * 4 + r;
                const int q = c * 4 + i;
                total += a[p] * b[q];
            }
            result[index] = total;
        }
    }
    for (int i = 0; i < 16; i++)
    {
        matrix[i] = result[i];
    }
}

void matrix::apply(float *data, const float *matrix, const int count, const int offset, const int stride) noexcept
{
    float vec[4] = {0, 0, 0, 1};
    for (int i = 0; i < count; i++)
    {
        float *d = data + offset + stride * i;
        vec[0] = *d++;
        vec[1] = *d++;
        vec[2] = *d++;
        vec_multiply(vec, matrix, vec);
        d = data + offset + stride * i;
        *d++ = vec[0];
        *d++ = vec[1];
        *d++ = vec[2];
    }
}

void matrix::frustum_planes(float planes[6][4], const int radius, const float *matrix) noexcept
{
    constexpr float znear = 0.125;
    const float zfar = radius * 32 + 64;
    const float *m = matrix;
    planes[0][0] = m[3] + m[0];
    planes[0][1] = m[7] + m[4];
    planes[0][2] = m[11] + m[8];
    planes[0][3] = m[15] + m[12];
    planes[1][0] = m[3] - m[0];
    planes[1][1] = m[7] - m[4];
    planes[1][2] = m[11] - m[8];
    planes[1][3] = m[15] - m[12];
    planes[2][0] = m[3] + m[1];
    planes[2][1] = m[7] + m[5];
    planes[2][2] = m[11] + m[9];
    planes[2][3] = m[15] + m[13];
    planes[3][0] = m[3] - m[1];
    planes[3][1] = m[7] - m[5];
    planes[3][2] = m[11] - m[9];
    planes[3][3] = m[15] - m[13];
    planes[4][0] = znear * m[3] + m[2];
    planes[4][1] = znear * m[7] + m[6];
    planes[4][2] = znear * m[11] + m[10];
    planes[4][3] = znear * m[15] + m[14];
    planes[5][0] = zfar * m[3] - m[2];
    planes[5][1] = zfar * m[7] - m[6];
    planes[5][2] = zfar * m[11] - m[10];
    planes[5][3] = zfar * m[15] - m[14];
}

void matrix::frustum(
    float *matrix, const float left, const float right, const float bottom,
    const float top, const float znear, const float zfar) noexcept
{
    const float temp = 2.0 * znear;
    const float temp2 = right - left;
    const float temp3 = top - bottom;
    const float temp4 = zfar - znear;
    matrix[0] = temp / temp2;
    matrix[1] = 0.0;
    matrix[2] = 0.0;
    matrix[3] = 0.0;
    matrix[4] = 0.0;
    matrix[5] = temp / temp3;
    matrix[6] = 0.0;
    matrix[7] = 0.0;
    matrix[8] = (right + left) / temp2;
    matrix[9] = (top + bottom) / temp3;
    matrix[10] = (-zfar - znear) / temp4;
    matrix[11] = -1.0;
    matrix[12] = 0.0;
    matrix[13] = 0.0;
    matrix[14] = (-temp * zfar) / temp4;
    matrix[15] = 0.0;
}

void matrix::perspective(
    float *matrix, const float fov, const float aspect,
    const float znear, const float zfar) noexcept
{
    const float ymax = znear * SDL_tanf(fov * MAT_PI_VAL / 360.0);
    const float xmax = ymax * aspect;
    frustum(matrix, -xmax, xmax, -ymax, ymax, znear, zfar);
}

void matrix::ortho(
    float *matrix,
    float left, float right, float bottom, float top, float znear, float zfar) noexcept
{
    matrix[0] = 2 / (right - left);
    matrix[1] = 0;
    matrix[2] = 0;
    matrix[3] = 0;
    matrix[4] = 0;
    matrix[5] = 2 / (top - bottom);
    matrix[6] = 0;
    matrix[7] = 0;
    matrix[8] = 0;
    matrix[9] = 0;
    matrix[10] = -2 / (zfar - znear);
    matrix[11] = 0;
    matrix[12] = -(right + left) / (right - left);
    matrix[13] = -(top + bottom) / (top - bottom);
    matrix[14] = -(zfar + znear) / (zfar - znear);
    matrix[15] = 1;
}

void matrix::set_2d(float *matrix, const int width, const int height) noexcept
{
    ortho(matrix, 0, width, 0, height, -1, 1);
}

void matrix::set_3d(
    float *matrix, const int width, const int height,
    const float x, const float y, const float z, const float rx, const float ry,
    const float fov, const int orthographic, const int radius) noexcept
{
    float a[16];
    float b[16];
    const float aspect = static_cast<float>(width) / height;
    const float zfar = radius * 32 + 64;

    identity(a);
    translate(b, -x, -y, -z);
    multiply(a, b, a);
    rotate(b, SDL_cosf(rx), 0, SDL_sinf(rx), ry);
    multiply(a, b, a);
    rotate(b, 0, 1, 0, -rx);
    multiply(a, b, a);
    if (orthographic)
    {
        const int size = orthographic;
        ortho(b, -size * aspect, size * aspect, -size, size, -zfar, zfar);
    }
    else
    {
        constexpr float znear = 0.125;
        perspective(b, fov, aspect, znear, zfar);
    }
    multiply(a, b, a);
    identity(matrix);
    multiply(matrix, a, matrix);
}

void matrix::set_item(float *matrix, const int width, const int height, const int scale) noexcept
{
    float a[16];
    float b[16];
    const float aspect = static_cast<float>(width) / height;
    const float size = 64.f * scale;
    const float box = height / size / 2.f;
    const float xoffset = 1.f - size / width * 2.f;
    const float yoffset = 1.f - size / height * 2.f;
    identity(a);
    rotate(b, 0, 1, 0, -MAT_PI_VAL / 4);
    multiply(a, b, a);
    rotate(b, 1, 0, 0, -MAT_PI_VAL / 10);
    multiply(a, b, a);
    ortho(b, -box * aspect, box * aspect, -box, box, -1, 1);
    multiply(a, b, a);
    translate(b, -xoffset, -yoffset, 0);
    multiply(a, b, a);
    identity(matrix);
    multiply(matrix, a, matrix);
}

void matrix::compute_sight_vector(const float rx, const float ry, float &vx, float &vy, float &vz) noexcept
{
    float m = SDL_cosf(ry);
    vx = SDL_cosf(rx - to_radians(90.0f)) * m;
    vy = SDL_sinf(ry);
    vz = SDL_sinf(rx - to_radians(90.0f)) * m;
}

void compute_motion_vector(const int flying, const int sz, const int sx, const float rx, const float ry,
                           float *vx, float *vy, float *vz) noexcept
{
    *vx = 0;
    *vy = 0;
    *vz = 0;
    if (!sz && !sx)
    {
        return;
    }
    const float strafe = SDL_atan2f(static_cast<float>(sz), static_cast<float>(sx));
    if (flying)
    {
        float m = SDL_cosf(ry);
        float y = SDL_sinf(ry);
        if (sx)
        {
            if (!sz)
            {
                y = 0;
            }
            m = 1;
        }
        if (sz > 0)
        {
            y = -y;
        }
        *vx = SDL_cosf(rx + strafe) * m;
        *vy = y;
        *vz = SDL_sinf(rx + strafe) * m;
    }
    else
    {
        *vx = SDL_cosf(rx + strafe);
        *vy = 0;
        *vz = SDL_sinf(rx + strafe);
    }
}
