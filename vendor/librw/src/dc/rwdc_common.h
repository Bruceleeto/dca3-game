#ifndef __RWDC_COMMON_H
#define __RWDC_COMMON_H

#include <tuple>
#include <limits>
#include <cmath>
#include <type_traits>
#include <algorithm>
#include <cstring>

#ifndef RW_DC
#   pragma warning(disable: 4244)  // int to float
#   pragma warning(disable: 4800)  // int to bool
#   pragma warning(disable: 4838)  // narrowing conversion
#   pragma warning(disable: 4996)  // POSIX names
#else
#   if !defined(DC_TEXCONV) && !defined(DC_SIM)
#       include <kos.h>
#       define DC_SH4
#       define VIDEO_MODE_WIDTH     static_cast<float>(vid_mode->width)
#       define VIDEO_MODE_HEIGHT    static_cast<float>(vid_mode->height)
#       define memcpy4              memcpy
#   else
#       ifdef DC_TEXCONV
#           define malloc_stats()
#       endif
#       include <dc/matrix.h>
#       include <dc/pvr.h>
#       include <kos/dbglog.h>
#       define VIDEO_MODE_WIDTH     640.0f
#       define VIDEO_MODE_HEIGHT    480.0f
#       define memcpy4              memcpy
#       define frsqrt(a)            (1.0f/sqrtf(a))
#       define dcache_pref_block(a)	__builtin_prefetch(a)
#       define F_PI                 M_PI
#       ifndef __always_inline 
#           define __always_inline  inline
#       endif
#   endif
#   ifdef PVR_TXRFMT_STRIDE
#       undef PVR_TXRFMT_STRIDE
#       define PVR_TXRFMT_STRIDE (1 << 25)
#   endif
    static_assert(PVR_TXRFMT_STRIDE == (1 << 25), 
                  "PVR_TXRFMT_STRIDE is bugged in your KOS version");
#endif

#define F_PI_2              (F_PI * 0.5f)
#define __hot               __attribute__((hot))
#define __cold              __attribute__((cold))
#define __icache_aligned    __attribute__((aligned(32)))

#define STRINGIFY(x)        #x
#define STR(x)              STRINGIFY(x)
#define CONCAT_(x,y)        x##y
#define CONCAT(x,y)         CONCAT_(x,y)
#define ARRAY_SIZE(array)   (sizeof(array) / sizeof(array[0]))

namespace rw {
    class Matrix;
}

namespace dc {

struct quaternion_t {
    float x, y, z, w;
};

template<bool BUILTIN=true>
__always_inline __hot constexpr float Sin(float x) {
#ifdef DC_SH4
    if constexpr(!BUILTIN)
        return fsin(x);
    else
#endif
    return sinf(x);
}

template<bool BUILTIN=true>
__always_inline __hot constexpr float Cos(float x) {
#ifdef DC_SH4
    if constexpr(!BUILTIN)
        return fcos(x);
    else
#endif
    return cosf(x);
}

template<bool BUILTIN=true>
__always_inline __hot constexpr auto SinCos(float x) {
#ifdef DC_SH4
    if constexpr(!BUILTIN) {
        std::pair<float, float> result;
        fsincosr(x, &std::get<0>(result), &std::get<1>(result));
        return result;
    } else
#endif
    return std::pair { Sin(x), Cos(x) };
}

__always_inline __hot constexpr float Tan(float x) { return tanf(x); }
__always_inline __hot constexpr float Atan(float x) { return atanf(x); }
__always_inline __hot constexpr float Atan2(float y, float x) { return atan2f(y, x); }
__always_inline __hot constexpr float Asin(float x) { return asinf(x);  }
__always_inline __hot constexpr float Acos(float x) { return acosf(x); }
__always_inline __hot constexpr float Abs(float x) { return fabsf(x); }
__always_inline __hot constexpr float Sqrt(float x) { return sqrtf(x); }
__always_inline __hot constexpr float RecipSqrt(float x, float y) { return x / Sqrt(y); }
__always_inline __hot constexpr float Pow(float x, float y) { return powf(x, y); }

template<bool FAST_APPROX=false>
__always_inline __hot constexpr float Floor(float x) {
#ifdef DC_SH4
    if(!std::is_constant_evaluated() && FAST_APPROX) {
        float output_float;
        unsigned int scratch_reg;
        unsigned int scratch_reg2;

        asm volatile (R"(
            mov     #0x4f, %[scratch]
            shll16  %[scratch]
            shll8   %[scratch]
            lds     %[scratch], fpul
            mov     #1, %[scratch2]
            fsts    fpul, %[float_out]
            fadd    %[floatx], %[float_out]
            rotr    %[scratch2]
            ftrc    %[float_out], fpul
            sts     fpul, %[scratch]
            add     %[scratch2], %[scratch]
            lds     %[scratch], fpul
            float   fpul, %[float_out]
        )"
        : [scratch] "=&r" (scratch_reg), [scratch2] "=&r" (scratch_reg2), [float_out] "=&f" (output_float)
        : [floatx] "f" (x)
        : "fpul", "t");

        return output_float;
    } else
#endif
    return floorf(x);
}

template<bool FAST_APPROX=false>
__always_inline __hot constexpr float Ceil(float x) {
#ifdef DC_SH4
    if(!std::is_constant_evaluated() && FAST_APPROX) {
        float output_float;
        unsigned int scratch_reg;
        unsigned int scratch_reg2;

        asm volatile (R"(
            mov     #0x4f, %[scratch]
            shll16  %[scratch]
            shll8   %[scratch]
            lds     %[scratch], fpul
            mov     #1, %[scratch2]
            fsts    fpul, %[float_out]
            fsub    %[floatx], %[float_out]
            rotr    %[scratch2]
            ftrc    %[float_out], fpul
            sts     fpul, %[scratch]
            add     %[scratch2], %[scratch]
            lds     %[scratch], fpul
            float   fpul, %[float_out]
            fneg    %[float_out]
        )"
        : [scratch] "=&r" (scratch_reg), [scratch2] "=&r" (scratch_reg2), [float_out] "=&f" (output_float)
        : [floatx] "f" (x)
        : "fpul", "t");

        return output_float;
    } else
#endif
    return ceilf(x);
}
__always_inline __hot constexpr float Fmac(auto a, auto b, auto c) { return a * b + c; }
__always_inline __hot constexpr float Lerp(float a, float b, float t) { return Fmac(t, (b - a), a); }
__always_inline __hot constexpr auto Max(auto a, auto b) { return ((a > b)? a : b); }
__always_inline __hot constexpr auto Min(auto a, auto b) { return ((a < b)? a : b); }

template<bool CHECK_ZERO=false>
__always_inline __hot constexpr float RecipSqrt(float x) { 
    if constexpr(CHECK_ZERO)
        if(x == 0.0f) 
            return 0.0f;
    
    return 1.0f / Sqrt(x); 
}

template<typename T, bool FAST_APPROX=true>
__always_inline __hot constexpr T Clamp(T v, auto low, auto high) {
    return std::clamp(v, static_cast<T>(low), static_cast<T>(high));
}

__always_inline constexpr auto Clamp2(auto v, auto center, auto radius) {
    return (v > center) ? Min(v, center + radius) : Max(v, center - radius);
}

template<bool FAST_APPROX=true, bool COPY_SIGN=true>
__always_inline __hot constexpr float Invert(float x) { 
    float value;
    
    if(!std::is_constant_evaluated() && FAST_APPROX) {
        value = RecipSqrt(x * x);

        if constexpr(COPY_SIGN)
            if(x < 0.0f)
                value = -value;

    } else value = 1.0f / x;

    return value;
} 

template<bool FAST_APPROX=true, bool COPY_SIGN=true>
__always_inline __hot constexpr float Div(float x, float y) { 
    if(FAST_APPROX && !std::is_constant_evaluated())
        return x * Invert<true, COPY_SIGN>(y);
    else
        return x / y;
}

template<bool FAST_APPROX=false, bool COPY_SIGN=true>
__always_inline __hot constexpr auto Norm(auto value, auto min, auto max) {
    auto numerator = Clamp(value, min, max) - min;
    auto denominator = (max - min);

    if(FAST_APPROX && !std::is_constant_evaluated())
        return Div<true, COPY_SIGN>(numerator, denominator);
    else
        return numerator / denominator;
}

#ifdef RW_DC
#   ifdef DC_SH4

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

#define mat_trans_nodiv_nomod_zerow(x, y, z, x2, y2, z2, w2) do { \
        register float __x __asm__("fr12") = (x); \
        register float __y __asm__("fr13") = (y); \
        register float __z __asm__("fr14") = (z); \
        register float __w __asm__("fr15") = 0.0f; \
        __asm__ __volatile__( "ftrv  xmtrx, fv12\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z), "=f" (__w) \
                              : "0" (__x), "1" (__y), "2" (__z), "3" (__w) ); \
        x2 = __x; y2 = __y; z2 = __z; w2 = __w; \
    } while(false)

#define mat_trans_w_nodiv_nomod(x, y, z, w) do { \
        register float __x __asm__("fr12") = (x); \
        register float __y __asm__("fr13") = (y); \
        register float __z __asm__("fr14") = (z); \
        register float __w __asm__("fr15") = 1.0f; \
        __asm__ __volatile__( "ftrv  xmtrx, fv12\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z), "=f" (__w) \
                              : "0" (__x), "1" (__y), "2" (__z), "3" (__w) ); \
        w = __w; \
    } while(false)

#define mat_trans_vec4_nodiv_nomod(x, y, z, w, x2, y2, z2, w2) { \
        register float __x __asm__("fr0") = (x); \
        register float __y __asm__("fr1") = (y); \
        register float __z __asm__("fr2") = (z); \
        register float __w __asm__("fr3") = (w); \
        __asm__ __volatile__( "ftrv  xmtrx, fv0\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z), "=f" (__w) \
                              : "0" (__x), "1" (__y), "2" (__z), "3" (__w) ); \
        x2 = __x; y2 = __y; z2 = __z; w2 = __w; \
    } while(false)

inline __hot __icache_aligned
void mat_load2(const matrix_t *mtx) {
    asm volatile(R"(
        fschg
        fmov.d	@%[mtx],xd0
        add	    #32,%[mtx]
        pref	@%[mtx]
        add	    #-(32-8),%[mtx]
        fmov.d	@%[mtx]+,xd2
        fmov.d	@%[mtx]+,xd4
        fmov.d	@%[mtx]+,xd6
        fmov.d	@%[mtx]+,xd8
        fmov.d	@%[mtx]+,xd10
        fmov.d	@%[mtx]+,xd12
        fmov.d	@%[mtx]+,xd14
        fschg
    )"
    : [mtx] "+r" (mtx));
}

inline __hot __icache_aligned
void mat_load_transpose(const matrix_t *mtx) {
    asm volatile(R"(
        frchg

        fmov.s  @%[mtx]+, fr0

        add     #32, %[mtx]
        pref    @%[mtx]
        add     #-(32 - 4), %[mtx]

        fmov.s  @%[mtx]+, fr4
        fmov.s  @%[mtx]+, fr8
        fmov.s  @%[mtx]+, fr12

        fmov.s  @%[mtx]+, fr1
        fmov.s  @%[mtx]+, fr5
        fmov.s  @%[mtx]+, fr9
        fmov.s  @%[mtx]+, fr13

        fmov.s  @%[mtx]+, fr2
        fmov.s  @%[mtx]+, fr6
        fmov.s  @%[mtx]+, fr10
        fmov.s  @%[mtx]+, fr14

        fmov.s  @%[mtx]+, fr3
        fmov.s  @%[mtx]+, fr7
        fmov.s  @%[mtx]+, fr11
        fmov.s  @%[mtx]+, fr15

        frchg
    )"
    : [mtx] "+r" (mtx));
}

inline __hot __icache_aligned
void mat_load_rows(const float *r1, const float *r2, const float *r3, const float *r4) {
    asm volatile (R"(
		frchg

        pref    @%1
		fmov.s  @%0+,fr0
		fmov.s  @%0+,fr4
		fmov.s  @%0+,fr8
		fmov.s  @%0, fr12

        pref    @%2
        fmov.s  @%1+,fr1
		fmov.s  @%1+,fr5
		fmov.s  @%1+,fr9
		fmov.s  @%1, fr13

        pref    @%3
        fmov.s  @%2+,fr2
		fmov.s  @%2+,fr6
		fmov.s  @%2+,fr10
		fmov.s  @%2,fr14

        fmov.s  @%3+,fr3
		fmov.s  @%3+,fr7
		fmov.s  @%3+,fr11
		fmov.s  @%3,fr15

		frchg
	)"
    : "+&r" (r1), "+&r" (r2), "+&r" (r3), "+&r" (r4));
}

inline __hot __icache_aligned
void mat_load_3x3_transpose(const matrix_t *mtx) {
    asm volatile(R"(
        frchg

        fmov.s  @%[mtx]+, fr0

        add     #32, %[mtx]
        pref    @%[mtx]
        add     #-(32 - 4), %[mtx]

        fmov.s  @%[mtx]+, fr4
        fmov.s  @%[mtx]+, fr8
        fldi0   fr12
        add     #4, %[mtx]

        fmov.s  @%[mtx]+, fr1
        fmov.s  @%[mtx]+, fr5
        fmov.s  @%[mtx]+, fr9
        fldi0   fr13
        add     #4, %[mtx]

        fmov.s  @%[mtx]+, fr2
        fmov.s  @%[mtx]+, fr6
        fmov.s  @%[mtx]+, fr10
        fldi0   fr14

        fldi0  fr3
        fldi0  fr7
        fmov   fr3, fr11
        fldi1  fr15

        frchg
    )"
    : [mtx] "+r" (mtx));
}

inline __hot __icache_aligned
void mat_invert_tranpose() {
	asm volatile(
		"frchg\n\t"
		"fneg	fr12\n\t"
		"fneg	fr13\n\t"
		"fneg	fr14\n\t"
		"fldi0	fr15\n\t"
		"fldi0	fr3\n\t"
		"fipr	fv12, fv0\n\t"
		"fldi0	fr7\n\t"
		"fipr	fv12, fv4\n\t"
		"fldi0	fr11\n\t"
		"fipr	fv12, fv8\n\t"

		"fmov	fr3, fr12\n\t"
		"fmov	fr7, fr13\n\t"
		"fmov	fr11, fr14\n\t"
		"fmov	fr1, fr15\n\t"
		"fmov	fr4, fr1\n\t"
		"fmov	fr15, fr4\n\t"
		"fmov	fr2, fr15\n\t"
		"fmov	fr8, fr2\n\t"
		"fmov	fr15, fr2\n\t"
		"fmov	fr6, fr15\n\t"
		"fmov	fr9, fr6\n\t"
		"fmov	fr15, fr9\n\t"

		"fldi0	fr3\n\t"
		"fldi0	fr7\n\t"
		"fldi0	fr11\n\t"
		"fldi1	fr15\n\t"
		"frchg\n"
		:
		:
		:);
}

inline __hot __icache_aligned
void mat_store2(matrix_t *mtx) {
    asm volatile(R"(
        fschg
        add	    #64-8,%[mtx]
        fmov.d	xd14,@%[mtx]
        add	    #-32,%[mtx]
        pref	@%[mtx]
        add	    #32,%[mtx]
        fmov.d	xd12,@-%[mtx]
        fmov.d	xd10,@-%[mtx]
        fmov.d	xd8,@-%[mtx]
        fmov.d	xd6,@-%[mtx]
        fmov.d	xd4,@-%[mtx]
        fmov.d	xd2,@-%[mtx]
        fmov.d	xd0,@-%[mtx]
        fschg
    )"
    : [mtx] "+&r" (mtx), "=m" (*mtx));
}

inline __hot __icache_aligned
void mat_identity2(void) {
    asm volatile(R"(
        frchg
        fldi1	fr0
        fschg
        fldi0	fr1
        fldi0	fr2
        fldi0	fr3
        fldi0	fr4
        fldi1	fr5
        fmov	dr2,dr6
        fmov	dr2,dr8
        fmov	dr0,dr10
        fmov	dr2,dr12
        fmov	dr4,dr14
        fschg
        frchg
    )");
}

inline __hot __icache_aligned
void mat_set_scale(float x, float y, float z) {
    asm volatile(R"(
        frchg
        fldi0	fr1
        fschg
        fldi0	fr2
        fldi0	fr3
        fldi0	fr4
        fmov	dr2, dr6
        fmov	dr2, dr8
        fldi0	fr11
        fmov	dr2, dr12
        fldi0	fr14
        fschg
        fmov.s	@%[x], fr0
        fmov.s	@%[y], fr5
        fmov.s	@%[z], fr10
        fldi1   fr15
        frchg
    )"
    :
    : [x] "r" (&x), [y] "r" (&y), [z] "r" (&z));
}

inline __hot __icache_aligned
void mat_set_scale(float s) {
    asm volatile(R"(
        frchg
        fldi0	fr1
        fschg
        fldi0	fr2
        fldi0	fr3
        fldi0	fr4
        fmov	dr2, dr6
        fmov	dr2, dr8
        fldi0	fr11
        fmov	dr2, dr12
        fldi0	fr14
        fschg
        fmov.s	@%[s], fr0
        fmov    fr0, fr5
        fmov	fr0, fr10
        fldi1   fr15
        frchg
    )"
    :
    : [s] "r" (&s));
}

// Don't multiply anything by fr3, since not loading from vector????
inline __hot __icache_aligned
void mat_apply_scale(float x, float y, float z) {
    asm volatile(R"(
        fschg
        fmov	xd0, dr4
        fmov	xd2, dr6
        fschg

        frchg
        fmov.s  @%[x], fr0
        fmov.s  @%[y], fr1
        fmov.s  @%[z], fr2

        fmul	fr0, fr4
        fmul	fr0, fr8
        fmul	fr0, fr12
        fmul	fr1, fr5
        fmul	fr1, fr9
        fmul	fr1, fr13
        fmul	fr2, fr6
        fmul	fr2, fr10
        fmul	fr2, fr14

        fschg
        fmov	dr4, xd0
        fmul	fr3, fr7
        fmov	dr6, xd2
        fmul	fr3, fr11
        fmov	xd4, dr4
        fmul	fr3, fr15
        fmov	xd6, dr6
        fschg

        fmul	fr4, fr0

        fmul	fr5, fr1
        fmul	fr6, fr2
        fmul	fr7, fr3

        fschg
        fmov	xd0, dr4
        fmov	xd2, dr6
        fschg

        frchg
    )"
    :
    : [x] "r" (&x), [y] "r" (&y), [z] "r" (&z)
    : "fr0", "fr1", "fr2");
}

inline __hot __icache_aligned
void mat_apply_translation(float x, float y, float z) {
    asm volatile(R"(
        fschg
        fmov	xd12, dr4
        fmov	xd14, dr6
        fschg

        fmov.s  @%[x], fr0
        fmov.s  @%[y], fr1
        fmov.s  @%[z], fr2

        fadd	fr0, fr4
        fadd	fr1, fr5
        fadd	fr2, fr6

        fschg
        fmov	dr4, xd12
        fmov	dr6, xd14
        fschg
    )"
    :
    : [x] "r" (&x), [y] "r" (&y), [z] "r" (&z)
    : "fr0", "fr1", "fr2", "fr4", "fr5", "fr6", "fr7");
}

inline __hot __icache_aligned
void mat_set_translation(float x, float y, float z) {
    asm volatile(R"(
        frchg
        fldi1	fr0
        fschg
        fldi0	fr1
        fldi0	fr2
        fldi0	fr3
        fldi0	fr4
        fldi1	fr5
        fmov	dr2,dr6
        fmov	dr2,dr8
        fmov	dr0,dr10
        fschg
        fmov.s  @%[x], fr12
        fmov.s  @%[y], fr13
        fmov.s  @%[z], fr14
        fldi1   fr15
        frchg
    )"
    :
    : [x] "r" (&x), [y] "r" (&y), [z] "r" (&z));
}

inline __hot __icache_aligned
void mat_transpose(void) {
    asm volatile (R"(
        frchg

        flds    fr1, fpul
        fmov    fr4, fr1
        fsts    fpul, fr4

        flds    fr2, fpul
        fmov    fr8, fr2
        fsts    fpul, fr8

        flds    fr3, fpul
        fmov    fr12, fr3
        fsts    fpul, fr12

        flds    fr6, fpul
        fmov    fr9, fr6
        fsts    fpul, fr9

        flds    fr7, fpul
        fmov    fr13, fr7
        fsts    fpul, fr13

        flds    fr11, fpUL
        fmov    fr14, fr11
        fsts    fpul, fr14

        frchg
    )"
    :
    :
    : "fpul");
}

__hot __icache_aligned inline
void mat_copy(matrix_t *dst, const matrix_t *src) {
    asm volatile(R"(
        fschg

        pref    @%[dst]
        fmov.d  @%[src]+, xd0
        fmov.d  @%[src]+, xd2
        fmov.d  @%[src]+, xd4
        fmov.d  @%[src]+, xd6
    
        pref    @%[src]
        add     #32, %[dst]

        fmov.d  xd6, @-%[dst]
        fmov.d  xd4, @-%[dst]
        fmov.d  xd2, @-%[dst]
        fmov.d  xd0, @-%[dst]

        add     #32, %[dst]
        pref    @%[dst]

        fmov.d  @%[src]+, xd0
        fmov.d  @%[src]+, xd2
        fmov.d  @%[src]+, xd4
        fmov.d  @%[src]+, xd6

        add     #32, %[dst]
        fmov.d  xd6, @-%[dst]
        fmov.d  xd4, @-%[dst]
        fmov.d  xd2, @-%[dst]
        fmov.d  xd0, @-%[dst]

        fschg
    )"
    : [dst] "+&r" (dst), [src] "+&r" (src), "=m" (*dst));
}

constexpr float fsca_scale = 10430.37835f;

__hot __icache_aligned inline
void mat_load_apply(const matrix_t* matrix1, const matrix_t* matrix2) {
    unsigned int prefetch_scratch;

    asm volatile (R"(
        mov     %[m1], %[prefscr]
        add     #32, %[prefscr]
        fschg
        pref    @%[prefscr]

        fmov.d  @%[m1]+, xd0
        fmov.d  @%[m1]+, xd2
        fmov.d  @%[m1]+, xd4
        fmov.d  @%[m1]+, xd6
        pref    @%[m1]
        fmov.d  @%[m1]+, xd8
        fmov.d  @%[m1]+, xd10
        fmov.d  @%[m1]+, xd12
        mov     %[m2], %[prefscr]
        add     #32, %[prefscr]
        fmov.d  @%[m1], xd14
        pref    @%[prefscr]

        fmov.d  @%[m2]+, dr0
        fmov.d  @%[m2]+, dr2
        fmov.d  @%[m2]+, dr4
        ftrv    xmtrx, fv0

        fmov.d  @%[m2]+, dr6
        fmov.d  @%[m2]+, dr8
        ftrv    xmtrx, fv4

        fmov.d  @%[m2]+, dr10
        fmov.d  @%[m2]+, dr12
        ftrv    xmtrx, fv8

        fmov.d  @%[m2], dr14
        fschg
        ftrv    xmtrx, fv12
        frchg
    )"
    : [m1] "+&r" (matrix1), [m2] "+r" (matrix2), [prefscr] "=&r" (prefetch_scratch)
    :
    : "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11", "fr12", "fr13", "fr14", "fr15");
}

__hot __icache_aligned inline void mat_set_rotate_x(float x) {
    x *= fsca_scale;
    asm volatile(R"(
        ftrc    %[x], fpul
		frchg
        fldi0   fr1
		fldi0	fr2
		fldi0	fr3
        fldi0	fr7
		fldi0	fr8
		fldi0	fr12
		fldi0	fr13
		fsca	fpul, dr0
		fldi0	fr4
		fldi0	fr11
		fldi0	fr14
		fldi1	fr15
		fmov	fr1, fr5
		fmov	fr1, fr10
		fmov	fr0, fr9
		fmov	fr0, fr6
        fneg    fr6
		fldi1	fr0
		fldi0	fr1
		frchg
    )"
    :
    : [x] "f" (x)
    : "fpul");
}

__hot __icache_aligned inline void mat_apply_rotate_x(float x) {
    x *= fsca_scale;
    asm volatile(R"(
        ftrc	%[x], fpul
        fsca 	fpul, dr4
        fldi0	fr8
        fldi0	fr11
        fmov	fr5, fr10
        fmov	fr4, fr9
        fneg	fr9
        ftrv	xmtrx, fv8
        fmov	fr4, fr6
        fldi0	fr7
        fldi0	fr4
        ftrv	xmtrx, fv4
        fschg
        fmov	dr8, xd8
        fmov	dr10, xd10
        fmov	dr4, xd4
        fmov	dr6, xd6
        fschg
    )"
    :
    : [x] "f"(x)
    : "fpul", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11");
}


__hot __icache_aligned inline void mat_set_rotate_y(float y) {
    y *= fsca_scale;
    asm volatile(R"(
        ftrc    %[y], fpul
        frchg
        fldi0	fr3
        fldi1	fr5
        fldi0	fr6
        fldi0	fr7
        fldi0	fr12
        fldi0	fr13
        fsca	fpul, dr0
        fldi0	fr4
        fldi0	fr9
        fldi0	fr11
        fldi0	fr14
        fldi1	fr15
        fmov	fr1, fr10
        fmov	fr0, fr8
        fneg	fr8
        fmov	fr0, fr2
        fmov	fr1, fr0
        fldi0	fr1
        frchg
    )"
    :
    : [y] "f" (y)
    : "fpul");
}

__hot __icache_aligned inline void mat_apply_rotate_y(float y) {
    y *= fsca_scale;
    asm volatile(R"(
        ftrc	%[y], fpul
        fsca    fpul, dr6
        fldi0	fr9
        fldi0	fr11
        fmov	fr6, fr8
        fmov	fr7, fr10
        ftrv	xmtrx, fv8
        fmov	fr7, fr4
        fldi0	fr5
        fneg	fr6
        fldi0	fr7
        ftrv	xmtrx, fv4
        fschg
        fmov	dr8, xd8
        fmov	dr10, xd10
        fmov	dr4, xd0
        fmov	dr6, xd2
        fschg
    )"
    :
    : [y] "f" (y)
    : "fpul", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11");
}

__hot __icache_aligned inline void mat_apply_rotate_z(float z) {
    z *= fsca_scale;
    asm volatile(R"(
        ftrc	%[z], fpul
        fsca    fpul, dr8
        fldi0	fr10
        fldi0	fr11
        fmov	fr8, fr5
        fneg	fr8
        ftrv	xmtrx, fv8
        fmov	fr9, fr4
        fschg
        fmov	dr10, dr6
        ftrv	xmtrx, fv4
        fmov	dr8, xd4
        fmov	dr10, xd6
        fmov	dr4, xd0
        fmov	dr6, xd2
        fschg
    )"
    :
    : [z] "f" (z)
    : "fpul", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11");
}

__hot __icache_aligned inline void mat_set_rotate_z(float z) {
    z *= fsca_scale; 
    asm volatile(R"(
        ftrc    %[z], fpul
		frchg
		fldi0	fr2
		fldi0	fr3
		fldi1	fr10
		fldi0	fr11
		fsca	fpul, dr4
		fschg
		fmov	dr2, dr6
		fmov	dr2, dr8
		fmov	dr2, dr12
		fldi0	fr14
		fldi1	fr15
		fschg
		fmov	fr5, fr0
		fmov	fr4, fr1
		fneg	fr1
		frchg
    )"
    :
    : [z] "f" (z)
    : "fpul");
}

__hot __icache_aligned inline void mat_set_rotate(float x, float y, float z) {
    mat_set_rotate_x(x);
    mat_apply_rotate_y(y);
    mat_apply_rotate_z(z);
}

__hot __icache_aligned inline void mat_apply_rotate(float x, float y, float z) {
    mat_apply_rotate_x(x);
    mat_apply_rotate_y(y);
    mat_apply_rotate_z(z);
}

//TODO: FIXME FOR VC (AND USE FTRV)
template<bool FAST_APPROX=false>
__hot constexpr inline void quat_mult(quaternion_t *r, const quaternion_t &q1, const quaternion_t &q2) {
    if(FAST_APPROX && !std::is_constant_evaluated()) {
    /*
        // reorder the coefficients so that q1 stays in constant order {x,y,z,w}
        // q2 then needs to be rotated after each inner product
        x =  (q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y) + (q1.w * q2.x);
        y = -(q1.x * q2.z) + (q1.y * q2.w) + (q1.z * q2.x) + (q1.w * q2.y);
        z =  (q1.x * q2.y) - (q1.y * q2.x) + (q1.z * q2.w) + (q1.w * q2.z);
        w = -(q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z) + (q1.w * q2.w);
    */
        // keep q1 in fv4
        register float q1x __asm__ ("fr4") = (q1.x);
        register float q1y __asm__ ("fr5") = (q1.y);
        register float q1z __asm__ ("fr6") = (q1.z);
        register float q1w __asm__ ("fr7") = (q1.w);

        // load q2 into fv8, use it to get the shuffled reorder into fv0
        register float q2x __asm__ ("fr8")  = (q2.x);
        register float q2y __asm__ ("fr9")  = (q2.y);
        register float q2z __asm__ ("fr10") = (q2.z);
        register float q2w __asm__ ("fr11") = (q2.w);

        // temporary operand / result in fv0
        register float t1x __asm__ ("fr0");
        register float t1y __asm__ ("fr1");
        register float t1z __asm__ ("fr2");
        register float t1w __asm__ ("fr3");

        // x =  (q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y) + (q1.w * q2.x);
        t1x = q2w;
        t1y = q2z;
        t1z = -q2y;
        t1w = q2w;
        __asm__ ("\n"
            " fipr	fv4,fv0\n"
            : "+f" (t1w)
            : "f" (q1x), "f" (q1y), "f" (q1z), "f" (q1w),
              "f" (t1x), "f" (t1y), "f" (t1z)
        );
        // x = t1w;  try to avoid the stall by not reading the fipr result immediately

        // y = -(q1.x * q2.z) + (q1.y * q2.w) + (q1.z * q2.x) + (q1.w * q2.y);
        t1x = -q2z;
        t1y = q2w;
        t1z = q2x;
        __atomic_thread_fence(1);
        r->x = t1w;   // get previous result
        t1w = q2y;
        __asm__ ("\n"
            "	fipr	fv4,fv0\n"
            : "+f" (t1w)
            : "f" (q1x), "f" (q1y), "f" (q1z), "f" (q1w),
              "f" (t1x), "f" (t1y), "f" (t1z)
        );
        //y = t1w;

        // z =  (q1.x * q2.y) - (q1.y * q2.x) + (q1.z * q2.w) + (q1.w * q2.z);
        t1x = q2y;
        t1y = -q2x;
        t1z = q2w;
        __atomic_thread_fence(1);
        r->y = t1w;   // get previous result
        t1w = q2z;
        __asm__ ("\n"
            "	fipr	fv4,fv0\n"
            : "+f" (t1w)
            : "f" (q1x), "f" (q1y), "f" (q1z), "f" (q1w),
              "f" (t1x), "f" (t1y), "f" (t1z)
        );
        //z = t1w;
        __atomic_thread_fence(1);

        // w = -(q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z) + (q1.w * q2.w);
        q2x = -q2x;
        q2y = -q2y;
        q2z = -q2z;
        __asm__ ("\n"
            "	fipr	fv4,fv8\n"
            : "+f" (q2w)
            : "f" (q1x), "f" (q1y), "f" (q1z), "f" (q1w),
              "f" (q2x), "f" (q2y), "f" (q2z)
        );

        __atomic_thread_fence(1);
        r->z = t1w;
        __atomic_thread_fence(1);
        r->w = q2w;
    } else {
        r->x = (q2.z * q1.y) - (q1.z * q2.y) + (q1.x * q2.w) + (q2.x * q1.w);
        r->y = (q2.x * q1.z) - (q1.x * q2.z) + (q1.y * q2.w) + (q2.y * q1.w);
        r->z = (q2.y * q1.x) - (q1.y * q2.x) + (q1.z * q2.w) + (q2.z * q1.w);
        r->w = (q2.w * q1.w) - (q2.x * q1.x) - (q2.y * q1.y) - (q2.z * q1.z);
    }
}

template<bool FAST_APPROX=true>
__always_inline constexpr float fipr2D(float x1, float y1, float x2, float y2) {
    if(FAST_APPROX && !std::is_constant_evaluated()) {
        register float v1x asm("fr0") = x1;
        register float v1y asm("fr1") = y1;
        register float res asm("fr3");

        register float v2x asm("fr4") = x2;
        register float v2y asm("fr5") = y2;

        asm volatile(R"(
                fldi0   fr2
                fldi0   fr3
                fipr    fv4, fv0
            )"
            : "=f" (res)
            : "f" (v1x), "f" (v1y), "f" (v2x), "f" (v2y));

        return res;
    } else {
        return (x1 * x2 + y1 * y2);
    }
}

#   else
#       ifdef DC_TEXCONV
#           define mat_apply(a)
#           define mat_load2(a)
#           define mat_store2(a)
#           define mat_identity2()
#           define mat_transform(a, b, c, d)
#           define pvr_fog_table_color(a,r,g,b)
#           define pvr_fog_table_linear(s,e)
#       else
#           define mat_load2(a)    mat_load(a)
#           define mat_store2(a)   mat_store(a)
#           define mat_identity2() mat_identity()
#       endif

#define mat_trans_single3_nomod(x_, y_, z_, x2, y2, z2) do { \
        vector_t tmp = { x_, y_, z_, 1.0f }; \
        mat_transform(&tmp, &tmp, 1, 0); \
        z2 = 1.0f / tmp.w; \
        x2 = tmp.x * z2; \
        y2 = tmp.y * z2; \
    } while(false)

#define mat_trans_single3_nodiv_nomod(x_, y_, z_, x2, y2, z2) do { \
        vector_t tmp = { x_, y_, z_, 1.0f }; \
        mat_transform(&tmp, &tmp, 1, 0); \
        z2 = tmp.z; \
        x2 = tmp.x; \
        y2 = tmp.y; \
    } while(false)

#define mat_trans_normal3_nomod(x_, y_, z_, x2, y2, z2) do { \
        vector_t tmp = { x_, y_, z_, 0.0f }; \
        mat_transform(&tmp, &tmp, 1, 0); \
        x2 = tmp.x; y2 = tmp.y; z2 = tmp.z; \
    } while(false)

#define mat_trans_nodiv_nomod(x_, y_, z_, x2, y2, z2, w2) do { \
        vector_t tmp1233123 = { x_, y_, z_, 1.0f }; \
        mat_transform(&tmp1233123, &tmp1233123, 1, 0); \
        x2 = tmp1233123.x; y2 = tmp1233123.y; z2 = tmp1233123.z; w2 = tmp1233123.w; \
    } while(false)

#define mat_trans_nodiv_nomod_zerow(x_, y_, z_, x2, y2, z2, w2) do { \
        vector_t tmp1233123 = { x_, y_, z_, 0.0f }; \
        mat_transform(&tmp1233123, &tmp1233123, 1, 0); \
        x2 = tmp1233123.x; y2 = tmp1233123.y; z2 = tmp1233123.z; w2 = tmp1233123.w; \
    } while(false)

#define mat_trans_w_nodiv_nomod(x_, y_, z_, w_) do { \
        vector_t tmp1233123 = { x_, y_, z_, 1.0f }; \
        mat_transform(&tmp1233123, &tmp1233123, 1, 0); \
        w_ = tmp1233123.w; \
    } while(false)

inline void mat_transpose(void) {
    matrix_t tmp;

    for(unsigned i = 0; i < 4; ++i)
        for(unsigned j = 0; j < 4; ++j)
            tmp[j][i] = XMTRX[i][j];
    
    mat_load(&tmp);
}

template<bool FAST_APPROX=true>
__hot inline void quat_mult(quaternion_t *r, const quaternion_t &q1, const quaternion_t &q2) {
    r->x = (q2.z * q1.y) - (q1.z * q2.y) + (q1.x * q2.w) + (q2.x * q1.w);
    r->y = (q2.x * q1.z) - (q1.x * q2.z) + (q1.y * q2.w) + (q2.y * q1.w);
    r->z = (q2.y * q1.x) - (q1.y * q2.x) + (q1.z * q2.w) + (q2.z * q1.w);
    r->w = (q2.w * q1.w) - (q2.x * q1.x) - (q2.y * q1.y) - (q2.z * q1.z);
}

__hot inline void mat_load_apply(const matrix_t* matrix1, const matrix_t* matrix2) {
    mat_load(matrix1);
    mat_apply(matrix2);
}

__always_inline __hot void mat_copy(matrix_t *dst, const matrix_t *src) {
    mat_load(src);
    mat_store(dst);
}

#endif

__hot inline void mat_mult(matrix_t *out, const matrix_t* matrix1, const matrix_t* matrix2) {
    mat_load_apply(matrix1, matrix2);
    mat_store2(out);
}

#endif

}

#endif /* RWDC_COMMON_H */