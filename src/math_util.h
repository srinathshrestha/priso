#ifndef MATH_UTIL_H
#define MATH_UTIL_H

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG2RAD(d) ((d) * M_PI / 180.0)

// 4x4 matrix in column-major order (OpenGL convention)
typedef struct { float m[16]; } Mat4;

static inline Mat4 mat4_identity(void) {
    Mat4 r = {0};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

static inline Mat4 mat4_mul(Mat4 a, Mat4 b) {
    Mat4 r = {0};
    for (int c = 0; c < 4; c++)
        for (int row = 0; row < 4; row++)
            for (int k = 0; k < 4; k++)
                r.m[c * 4 + row] += a.m[k * 4 + row] * b.m[c * 4 + k];
    return r;
}

static inline Mat4 mat4_translate(float x, float y, float z) {
    Mat4 r = mat4_identity();
    r.m[12] = x; r.m[13] = y; r.m[14] = z;
    return r;
}

static inline Mat4 mat4_scale(float x, float y, float z) {
    Mat4 r = {0};
    r.m[0] = x; r.m[5] = y; r.m[10] = z; r.m[15] = 1.0f;
    return r;
}

static inline Mat4 mat4_rotate_x(float deg) {
    float r = (float)DEG2RAD(deg);
    float c = cosf(r), s = sinf(r);
    Mat4 m = mat4_identity();
    m.m[5] = c;  m.m[6] = s;
    m.m[9] = -s; m.m[10] = c;
    return m;
}

static inline Mat4 mat4_rotate_y(float deg) {
    float r = (float)DEG2RAD(deg);
    float c = cosf(r), s = sinf(r);
    Mat4 m = mat4_identity();
    m.m[0] = c;  m.m[2] = -s;
    m.m[8] = s;  m.m[10] = c;
    return m;
}

static inline Mat4 mat4_rotate_z(float deg) {
    float r = (float)DEG2RAD(deg);
    float c = cosf(r), s = sinf(r);
    Mat4 m = mat4_identity();
    m.m[0] = c;  m.m[1] = s;
    m.m[4] = -s; m.m[5] = c;
    return m;
}

static inline Mat4 mat4_perspective(float fov_deg, float aspect, float near, float far) {
    float f = 1.0f / tanf((float)DEG2RAD(fov_deg) / 2.0f);
    Mat4 r = {0};
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (far + near) / (near - far);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * far * near) / (near - far);
    return r;
}

static inline Mat4 mat4_ortho(float left, float right, float bottom, float top, float near, float far) {
    Mat4 r = {0};
    r.m[0]  = 2.0f / (right - left);
    r.m[5]  = 2.0f / (top - bottom);
    r.m[10] = -2.0f / (far - near);
    r.m[12] = -(right + left) / (right - left);
    r.m[13] = -(top + bottom) / (top - bottom);
    r.m[14] = -(far + near) / (far - near);
    r.m[15] = 1.0f;
    return r;
}

static inline Mat4 mat4_lookat(float ex, float ey, float ez,
                                float cx, float cy, float cz,
                                float ux, float uy, float uz) {
    float fx = cx - ex, fy = cy - ey, fz = cz - ez;
    float fl = sqrtf(fx*fx + fy*fy + fz*fz);
    if (fl < 1e-8f) return mat4_identity();
    fx /= fl; fy /= fl; fz /= fl;

    // side = f x up
    float sx = fy * uz - fz * uy;
    float sy = fz * ux - fx * uz;
    float sz = fx * uy - fy * ux;
    float sl = sqrtf(sx*sx + sy*sy + sz*sz);
    if (sl < 1e-8f) return mat4_identity();
    sx /= sl; sy /= sl; sz /= sl;

    // u = s x f
    float uux = sy * fz - sz * fy;
    float uuy = sz * fx - sx * fz;
    float uuz = sx * fy - sy * fx;

    Mat4 r = mat4_identity();
    r.m[0] = sx;  r.m[4] = sy;  r.m[8]  = sz;
    r.m[1] = uux; r.m[5] = uuy; r.m[9]  = uuz;
    r.m[2] = -fx; r.m[6] = -fy; r.m[10] = -fz;
    r.m[12] = -(sx*ex + sy*ey + sz*ez);
    r.m[13] = -(uux*ex + uuy*ey + uuz*ez);
    r.m[14] = (fx*ex + fy*ey + fz*ez);
    return r;
}

// Normalize a 3-component vector
static inline void vec3_normalize(float *x, float *y, float *z) {
    float l = sqrtf((*x)*(*x) + (*y)*(*y) + (*z)*(*z));
    if (l > 0.0001f) { *x /= l; *y /= l; *z /= l; }
}

#endif
