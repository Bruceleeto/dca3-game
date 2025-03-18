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

#define F_PI_2          (F_PI * 0.5f)
#define __hot           __attribute__((hot))
#define __cold          __attribute__((cold))

#define STRINGIFY(x)    #x
#define STR(x)          STRINGIFY(x)
#define CONCAT_(x,y)    x##y
#define CONCAT(x,y)     CONCAT_(x,y)

namespace rw {
    class Matrix;
}

namespace dc {

struct quaternion_t {
    float x, y, z, w;
};

__always_inline __hot constexpr float Sin(float x) { return sinf(x); }
__always_inline __hot constexpr float Cos(float x) { return cosf(x); }
__always_inline __hot constexpr auto  SinCos(float x) { return std::pair { Sin(x), Cos(x) }; }
__always_inline __hot constexpr float Abs(float x) { return fabsf(x); }
__always_inline __hot constexpr float Sqrt(float x) { return sqrtf(x); }
__always_inline __hot constexpr float RecipSqrt(float x, float y) { return x / Sqrt(y); }
__always_inline __hot constexpr float Pow(float x, float y) { return powf(x, y); }
__always_inline __hot constexpr float Floor(float x) { return floorf(x); }
__always_inline __hot constexpr float Ceil(float x) { return ceilf(x); }
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

template<typename T>
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

template<bool FAST_APPROX=false, bool FAST_DIV=false, bool DIV_COPY_SIGN=false>
__always_inline __hot constexpr float Tan(float x) { 
    if(!std::is_constant_evaluated() && FAST_APPROX) {
        constexpr float pisqby4 = 2.4674011002723397f;
        constexpr float adjpisqby4 = 2.471688400562703f;
        constexpr float adj1minus8bypisq = 0.189759681063053f;
        float xsq = x * x;
        
        return x * Div<FAST_DIV, DIV_COPY_SIGN>(adjpisqby4 - adj1minus8bypisq * xsq, 
                                                pisqby4 - xsq);
    } else
        return tanf(x); 
}

template<bool FAST_APPROX=false>
__always_inline __hot constexpr float Atan(float x) { 
    if(FAST_APPROX && !std::is_constant_evaluated()) {
        constexpr float a[3] = { // 
            0.998418889819911f, -2.9993501171084700E-01f, 0.0869142852883849f};
        float xx = x * x;
        return ((a[2] * xx + a[1]) * xx + a[0]) * x;
    } else return atanf(x); 
}

template<bool FAST_APPROX=false>
__hot constexpr float Atan2(float y, float x) {
    if(FAST_APPROX && !std::is_constant_evaluated()) {
#if 0
        constexpr float halfpi_i754 = M_PI * 0.5f;
        constexpr float quarterpi_i754 = M_PI * 0.25f;
        // kludge to prevent 0/0 condition
        float abs_y = Abs(y) + std::numeric_limits<float>::epsilon();
        float absy_plus_absx = abs_y + Abs(x);
        float inv_absy_plus_absx = Invert<true, true>(absy_plus_absx);
        float angle = halfpi_i754 - copysignf(quarterpi_i754, x);
        float r = (x - copysignf(abs_y, x)) * inv_absy_plus_absx;
        angle += (0.1963f * r * r - 0.9817f) * r;
        return copysignf(angle, y);
#else
    // Ensure input is in [-1, +1]
    bool swap = fabs(x) < fabs(y);
    float atan_input = (swap ? x : y) / (swap ? y : x);

    // Approximate atan
    float res = Atan<true>(atan_input);

    // If swapped, adjust atan output
    res = swap ? (atan_input >= 0.0f ? F_PI_2 : -F_PI_2) - res : res;
    // Adjust quadrants
    if      (x >= 0.0f && y >= 0.0f) {}                     // 1st quadrant
    else if (x <  0.0f && y >= 0.0f) { res =  F_PI + res; } // 2nd quadrant
    else if (x <  0.0f && y <  0.0f) { res = -F_PI + res; } // 3rd quadrant
    else if (x >= 0.0f && y <  0.0f) {}                     // 4th quadrant

    // Store result
    return res;
#endif
    } else return atan2f(y, x);
}

template<bool FAST_APPROX=false>
__always_inline __hot constexpr float Asin(float x) { 
    if(FAST_APPROX && !std::is_constant_evaluated()) {
        Atan(Div<true, false>(x, Sqrt(1.0f-(x*x))));
    } else return asinf(x); 
}

template<bool FAST_APPROX=false>
__always_inline __hot constexpr float Acos(float x) {
    if(FAST_APPROX && !std::is_constant_evaluated()) {
         return (-0.69813170079773212f * x * x - 0.87266462599716477f) * x + 1.5707963267948966f;
    } else return acosf(x); 
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

#define mat_trans_vec3(x, y, z) do { \
        register float __x __asm__("fr12") = (x); \
        register float __y __asm__("fr13") = (y); \
        register float __z __asm__("fr14") = (z); \
        __asm__ __volatile__( \
                              "fldi0 fr15\n" \
                              "ftrv  xmtrx, fv12\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z) \
                              : "0" (__x), "1" (__y), "2" (__z) \
                              : "fr15" ); \
        x = __x; y = __y; z = __z; \
    } while(false)


#define mat_trans_vec3_nomod(x, y, z, x2, y2, z2) { \
        register float __x __asm__("fr12") = (x); \
        register float __y __asm__("fr13") = (y); \
        register float __z __asm__("fr14") = (z); \
        __asm__ __volatile__( \
                              "fldi0 fr15\n" \
                              "ftrv  xmtrx, fv12\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z) \
                              : "0" (__x), "1" (__y), "2" (__z) \
                              : "fr15" ); \
        x2 = __x; y2 = __y; z2 = __z; \
    }

// no declspec naked, so can't do rts / fschg. instead compiler pads with nop?
__always_inline __hot void mat_load_3x3(const matrix_t* mtx) {
    __asm__ __volatile__ (
        R"(
            fschg
            frchg

            fmov        @%[mtx]+, dr0
            fldi0 		fr12

            fmov        @%[mtx]+, dr2
            fldi0 		fr13

            fmov        @%[mtx]+, dr4
            fldi0	    fr3

            fmov        @%[mtx]+, dr6
            fmov        dr12, dr14

            fmov        @%[mtx]+, dr8
            fldi0	    fr7

            fmov        @%[mtx]+, dr10
            fldi0	    fr11

            fschg
            frchg
        )"
        : [mtx] "+r" (mtx)
    );
}

// sets pos.w to 1
__always_inline __hot void rw_mat_load_4x4(const rw::Matrix* mtx) {
    __asm__ __volatile__ (
        R"(
            fschg
            frchg
            fmov        @%[mtx]+, dr0

            fmov        @%[mtx]+, dr2
            fmov        @%[mtx]+, dr4
            fmov        @%[mtx]+, dr6
            fmov        @%[mtx]+, dr8
            fmov        @%[mtx]+, dr10
            fmov        @%[mtx]+, dr12
            fmov        @%[mtx]+, dr14
            fldi1 	 	fr15

            fschg
            frchg
        )"
        : [mtx] "+r" (mtx)
    );
}

__always_inline __hot void mat_transpose(void) {
    asm volatile (
        "frchg\n\t" // fmov for singles only works on front bank
        // FR0, FR5, FR10, and FR15 are already in place
        // swap FR1 and FR4
        "flds FR1, FPUL\n\t"
        "fmov FR4, FR1\n\t"
        "fsts FPUL, FR4\n\t"
        // swap FR2 and FR8
        "flds FR2, FPUL\n\t"
        "fmov FR8, FR2\n\t"
        "fsts FPUL, FR8\n\t"
        // swap FR3 and FR12
        "flds FR3, FPUL\n\t"
        "fmov FR12, FR3\n\t"
        "fsts FPUL, FR12\n\t"
        // swap FR6 and FR9
        "flds FR6, FPUL\n\t"
        "fmov FR9, FR6\n\t"
        "fsts FPUL, FR9\n\t"
        // swap FR7 and FR13
        "flds FR7, FPUL\n\t"
        "fmov FR13, FR7\n\t"
        "fsts FPUL, FR13\n\t"
        // swap FR11 and FR14
        "flds FR11, FPUL\n\t"
        "fmov FR14, FR11\n\t"
        "fsts FPUL, FR14\n\t"
        // restore XMTRX to back bank
        "frchg\n"
        : // no outputs
        : // no inputs
        : "fpul" // clobbers
    );
}


template<bool FAST_APPROX=true>
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

__hot inline void mat_load_apply(const matrix_t* matrix1, const matrix_t* matrix2) {
    unsigned int prefetch_scratch;

    asm volatile (
        "mov %[bmtrx], %[pref_scratch]\n\t" // (MT)
        "add #32, %[pref_scratch]\n\t" // offset by 32 (EX - flow dependency, but 'add' is actually parallelized since 'mov Rm, Rn' is 0-cycle)
        "fschg\n\t" // switch fmov to paired moves (note: only paired moves can access XDn regs) (FE)
        "pref @%[pref_scratch]\n\t" // Get a head start prefetching the second half of the 64-byte data (LS)
        // back matrix
        "fmov.d @%[bmtrx]+, XD0\n\t" // (LS)
        "fmov.d @%[bmtrx]+, XD2\n\t"
        "fmov.d @%[bmtrx]+, XD4\n\t"
        "fmov.d @%[bmtrx]+, XD6\n\t"
        "pref @%[fmtrx]\n\t" // prefetch fmtrx now while we wait (LS)
        "fmov.d @%[bmtrx]+, XD8\n\t" // bmtrx prefetch should work for here
        "fmov.d @%[bmtrx]+, XD10\n\t"
        "fmov.d @%[bmtrx]+, XD12\n\t"
        "mov %[fmtrx], %[pref_scratch]\n\t" // (MT)
        "add #32, %[pref_scratch]\n\t" // store offset by 32 in r0 (EX - flow dependency, but 'add' is actually parallelized since 'mov Rm, Rn' is 0-cycle)
        "fmov.d @%[bmtrx], XD14\n\t"
        "pref @%[pref_scratch]\n\t" // Get a head start prefetching the second half of the 64-byte data (LS)
        // front matrix
        // interleave loads and matrix multiply 4x4
        "fmov.d @%[fmtrx]+, DR0\n\t"
        "fmov.d @%[fmtrx]+, DR2\n\t"
        "fmov.d @%[fmtrx]+, DR4\n\t" // (LS) want to issue the next one before 'ftrv' for parallel exec
        "ftrv XMTRX, FV0\n\t" // (FE)

        "fmov.d @%[fmtrx]+, DR6\n\t"
        "fmov.d @%[fmtrx]+, DR8\n\t"
        "ftrv XMTRX, FV4\n\t"

        "fmov.d @%[fmtrx]+, DR10\n\t"
        "fmov.d @%[fmtrx]+, DR12\n\t"
        "ftrv XMTRX, FV8\n\t"

        "fmov.d @%[fmtrx], DR14\n\t" // (LS, but this will stall 'ftrv' for 3 cycles)
        "fschg\n\t" // switch back to single moves (and avoid stalling 'ftrv') (FE)
        "ftrv XMTRX, FV12\n\t" // (FE)
        // Save output in XF regs
        "frchg\n"
        : [bmtrx] "+&r" ((unsigned int)matrix1), [fmtrx] "+r" ((unsigned int)matrix2), [pref_scratch] "=&r" (prefetch_scratch) // outputs, "+" means r/w, "&" means it's written to before all inputs are consumed
        : // no inputs
        : "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11", "fr12", "fr13", "fr14", "fr15" // clobbers (GCC doesn't know about back bank, so writing to it isn't clobbered)
    );
}

#   else
#       ifdef DC_TEXCONV
#           define mat_transform(a, b, c, d)
#           define mat_apply(a)
#           define mat_load(a)
#           define mat_store(a)
#           define mat_identity(a)
#           define pvr_fog_table_color(a,r,g,b)
#           define pvr_fog_table_linear(s,e)
#       endif

#define mat_trans_vec3(x_, y_, z_) do { \
        vector_t tmp = { x_, y_, z_, 0.0f }; \
        mat_transform(&tmp, &tmp, 1, 0); \
        x_ = tmp.x; y_ = tmp.y; z_ = tmp.z; \
    } while(false)

#define mat_trans_vec3_nomod(x_, y_, z_, x2, y2, z2) do { \
        vector_t tmp = { x_, y_, z_, 0.0f }; \
        mat_transform(&tmp, &tmp, 1, 0); \
        x2 = tmp.x; y2 = tmp.y; z2 = tmp.z; \
    } while(false)

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

inline void mat_load_3x3(const matrix_t* mtx) {
	memcpy(XMTRX, mtx, sizeof(matrix_t));
	XMTRX[0][3] = 0.0f;
	XMTRX[1][3] = 0.0f;
	XMTRX[2][3] = 0.0f;

	XMTRX[3][0] = 0.0f;
	XMTRX[3][1] = 0.0f;
	XMTRX[3][2] = 0.0f;
	XMTRX[3][3] = 0.0f;
}

inline void rw_mat_load_4x4(const rw::Matrix* mtx) {
	memcpy(XMTRX, mtx, sizeof(matrix_t));
	XMTRX[3][3] = 1.0f;
}

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

#endif

__hot inline void mat_mult(matrix_t *out, const matrix_t* matrix1, const matrix_t* matrix2) {
    mat_load_apply(matrix1, matrix2);
    mat_store(out);
}

__always_inline __hot void mat_copy(matrix_t *dst, const matrix_t *src) {
    mat_load(src);
    mat_store(dst);
}

#if defined(DC_SH4)
inline uint8_t* OCRAM = (uint8_t*)0x7c001000;
#else
alignas(32) inline uint8_t OCRAM[32 * 256];
#endif

inline void ocram_enter() {
	#if defined(DC_SH4)
	auto mask = irq_disable();
	dcache_purge_all();
	volatile uint32_t * CCN_CCR = (uint32_t *)0xFF00001C;
	*CCN_CCR |= (1 << 5); // enable OCR (ORA)
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");

	irq_restore(mask);
	#endif
}

inline void ocram_leave() {
	#if defined(DC_SH4)
	auto mask = irq_disable();
	dcache_inval_range(0x92000000, 8192);
	dcache_purge_all();
	volatile uint32_t * CCN_CCR = (uint32_t *)0xFF00001C;
	*CCN_CCR &= ~( 1 << 5); // disable OCR (ORA)
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	irq_restore(mask);
	#endif
}


#endif

}

#endif /* RWDC_COMMON_H */