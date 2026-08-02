#ifndef MATRIX_H
#define MATRIX_H

class matrix
{
public:
    static constexpr auto MAT_PI_VAL =
#ifndef M_PI
        3.14159265358979323846f;
#else
        M_PI;
#endif

    static constexpr float to_degrees(float radians) noexcept
    {
        return radians * 180.0f / MAT_PI_VAL;
    }

    static constexpr float to_radians(float degrees) noexcept
    {
        return degrees * MAT_PI_VAL / 180.0f;
    }

    template <typename T>
    static constexpr auto clamp(T value, T min, T max) noexcept
    {
        if (value < min)
        {
            return min;
        }
        else if (value > max)
        {
            return max;
        }
        else
        {
            return value;
        }
    }

    template <typename Sign>
    static constexpr auto sign(Sign value) noexcept
    {
        return (value > 0) - (value < 0);
    }

    static void normalize(float *x, float *y, float *z) noexcept;

    static void identity(float *matrix) noexcept;

    static void translate(float *matrix, float dx, float dy, float dz) noexcept;

    static void rotate(float *matrix, float x, float y, float z, float angle) noexcept;

    static void vec_multiply(float *vector, const float *a, const float *b) noexcept;

    static void multiply(float *matrix, const float *a, const float *b) noexcept;

    static void apply(float *data, const float *matrix, int count, int offset, int stride) noexcept;

    static void frustum_planes(float planes[6][4], int radius, const float *matrix) noexcept;

    static void frustum(
        float *matrix, float left, float right, float bottom,
        float top, float znear, float zfar) noexcept;

    static void perspective(
        float *matrix, float fov, float aspect,
        float znear, float zfar) noexcept;

    static void ortho(
        float *matrix,
        float left, float right, float bottom, float top, float znear, float zfar) noexcept;

    static void set_2d(float *matrix, int width, int height) noexcept;

    static void set_3d(
        float *matrix, int width, int height,
        float x, float y, float z, float rx, float ry,
        float fov, int orthographic, int radius) noexcept;

    static void set_item(float *matrix, int width, int height, int scale) noexcept;

    static void compute_sight_vector(float rx, float ry, float &vx, float &vy, float &vz) noexcept;

    static void compute_motion_vector(const int flying, const int sz, const int sx, const float rx, const float ry,
                                      float *vx, float *vy, float *vz) noexcept;
}; // class

#endif
