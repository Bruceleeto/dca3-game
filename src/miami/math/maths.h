#pragma once

#include "src/common_defines.h"

#include <dc/matrix.h>

#ifdef DC_SH4

#define mat_trans_nodiv_nomod(x, y, z, x2, y2, z2, w2) do { \
        register float __x __asm__("fr12") = (x); \
        register float __y __asm__("fr13") = (y); \
        register float __z __asm__("fr14") = (z); \
        register float __w __asm__("fr15") = 1.0f; \
        __asm__ __volatile__( "ftrv  xmtrx, fv12\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z), "=f" (__w) \
                              : "0" (__x), "1" (__y), "2" (__z), "3" (__w) ); \
        x2 = __x; y2 = __y; z2 = __z; w2 = __w; \
    } while(false)

#else

#define mat_trans_nodiv_nomod(x_, y_, z_, x2, y2, z2, w2) do { \
		vector_t tmp = { x_, y_, z_, 1.0f }; \
		mat_transform(&tmp, &tmp, 1, 0); \
		x2 = tmp.x; y2 = tmp.y; z2 = tmp.z; w2 = tmp.w; \
	} while(false)
#endif


// wrapper around float versions of functions
// in gta they are in CMaths but that makes the code rather noisy

inline float Sin(float x) { return sinf(x); }
inline float Asin(float x) { return asinf(x); }
inline float Cos(float x) { return cosf(x); }
inline float Acos(float x) { return acosf(x); }
inline float Tan(float x) { return tanf(x); }
inline float Atan(float x) { return atanf(x); }
inline float Atan2(float y, float x) { return atan2f(y, x); }
inline float Abs(float x) { return fabsf(x); }
inline float Sqrt(float x) { return sqrtf(x); }
inline float RecipSqrt(float x, float y) { return x/Sqrt(y); }
inline float RecipSqrt(float x) { return RecipSqrt(1.0f, x); }
inline float Pow(float x, float y) { return powf(x, y); }
inline float Floor(float x) { return floorf(x); }
inline float Ceil(float x) { return ceilf(x); }
