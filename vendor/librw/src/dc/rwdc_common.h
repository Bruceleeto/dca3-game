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

__always_inline __hot constexpr float Sin(float x) { return sinf(x); }
__always_inline __hot constexpr float Cos(float x) { return cosf(x); }
__always_inline __hot constexpr auto  SinCos(float x) { return std::pair { Sin(x), Cos(x) }; }
__always_inline __hot constexpr float Tan(float x) { return tanf(x); }
__always_inline __hot constexpr float Atan(float x) { return atanf(x); }
__always_inline __hot constexpr float Atan2(float y, float x) { return atan2f(y, x); }
__always_inline __hot constexpr float Asin(float x) { return asinf(x);  }
__always_inline __hot constexpr float Acos(float x) { return acosf(x); }
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

inline __hot __icache_aligned void mat_load2(const matrix_t* mtx) {
    asm volatile(
        R"(
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
        : [mtx] "+r" (mtx)
        :
        :
    );
}

inline __hot __icache_aligned void mat_store2(matrix_t *mtx) {
    asm volatile(
        R"(
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
        : [mtx] "+&r" (mtx), "=m" (*mtx)
        :
        :
    );
}

inline __hot __icache_aligned void mat_identity2(void) {
    asm volatile(
        R"(
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
        )"
    );
}

inline __hot __icache_aligned void mat_set_scale(float x, float y, float z) {
    asm volatile(
        R"(
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
            frchg
            fmov    %[x], xf0
            fmov    %[y], xf5
            fmov    %[z], xf10
        )"
        :
        : [x] "r" (x), [y] "r" (y), [z] "r" (z)
        :
    );
}

// no declspec naked, so can't do rts / fschg. instead compiler pads with nop?
inline __hot void mat_load_3x3(const rw::Matrix* mtx) {
    __asm__ __volatile__ (
        R"(
            fschg
            frchg

            fmov        @%[mtx]+, dr0

            fldi0 		fr12
            fldi0 		fr13

            fmov        @%[mtx]+, dr2
            fmov        @%[mtx]+, dr4
            fmov        @%[mtx]+, dr6
            fmov        @%[mtx]+, dr8
            fmov        @%[mtx]+, dr10

            fldi0	    fr3
            fldi0	    fr7
            fldi0	    fr11
            fmov        dr12, dr14

            fschg
            frchg
        )"
        : [mtx] "+r" (mtx)
    );
}

// sets pos.w to 1
inline __hot void mat_load_4x4(const rw::Matrix* mtx) {
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

__hot inline void mat_transpose(void) {
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

__hot __icache_aligned inline void mat_copy(matrix_t *dst, const matrix_t *src) {
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
    )": [dst] "+&r" (dst), [src] "+&r" (src), "=m" (*dst)
      :
      :);
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

__hot inline void mat_apply_rotate_x(float x) {
    x *= 10430.37835f;
    asm volatile(
        "ftrc	%[a], fpul\n\t"
        "fsca 	fpul, dr4\n\t"
        "fldi0	fr8\n\t"
        "fldi0	fr11\n\t"
        "fmov	fr5, fr10\n\t"
        "fmov	fr4, fr9\n\t"
        "fneg	fr9\n\t"
        "ftrv	xmtrx, fv8\n\t"
        "fmov	fr4, fr6\n\t"
        "fldi0	fr7\n\t"
        "fldi0	fr4\n\t"
        "ftrv	xmtrx, fv4\n\t"
        "fschg\n\t"
        "fmov	dr8, xd8\n\t"
        "fmov	dr10, xd10\n\t"
        "fmov	dr4, xd4\n\t"
        "fmov	dr6, xd6\n\t"
        "fschg\n"
        :
        : [a] "f"(x)
        : "fpul", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11");
}

__hot inline void mat_apply_rotate_y(float y) {
    y *= 10430.37835f;
    asm volatile(
        "ftrc	%[a], fpul\n\t"
        "fsca   fpul, dr6\n\t"
        "fldi0	fr9\n\t"
        "fldi0	fr11\n\t"
        "fmov	fr6, fr8\n\t"
        "fmov	fr7, fr10\n\t"
        "ftrv	xmtrx, fv8\n\t"
        "fmov	fr7, fr4\n\t"
        "fldi0	fr5\n\t"
        "fneg	fr6\n\t"
        "fldi0	fr7\n\t"
        "ftrv	xmtrx, fv4\n\t"
        "fschg\n\t"
        "fmov	dr8, xd8\n\t"
        "fmov	dr10, xd10\n\t"
        "fmov	dr4, xd0\n\t"
        "fmov	dr6, xd2\n\t"
        "fschg\n"
        :
        : [a] "f"(y)
        : "fpul", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11");
}

__hot inline void mat_apply_rotate_z(float z) {
    z *= 10430.37835f;
    asm volatile(
        "ftrc	%[a], fpul\n\t"
        "fsca   fpul, dr8\n\t"
        "fldi0	fr10\n\t"
        "fldi0	fr11\n\t"
        "fmov	fr8, fr5\n\t"
        "fneg	fr8\n\t"
        "ftrv	xmtrx, fv8\n\t"
        "fmov	fr9, fr4\n\t"
        "fschg\n\t"
        "fmov	dr10, dr6\n\t"
        "ftrv	xmtrx, fv4\n\t"
        "fmov	dr8, xd4\n\t"
        "fmov	dr10, xd6\n\t"
        "fmov	dr4, xd0\n\t"
        "fmov	dr6, xd2\n\t"
        "fschg\n"
        :
        : [a] "f"(z)
        : "fpul", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11");
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

inline void mat_load_3x3(const rw::Matrix* mtx) {
    memcpy(XMTRX, mtx, sizeof(matrix_t));
    XMTRX[0][3] = 0.0f;
    XMTRX[1][3] = 0.0f;
    XMTRX[2][3] = 0.0f;

    XMTRX[3][0] = 0.0f;
    XMTRX[3][1] = 0.0f;
    XMTRX[3][2] = 0.0f;
    XMTRX[3][3] = 0.0f;
}

inline void mat_load_4x4(const rw::Matrix* mtx) {
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