#ifndef SF_BIONIC_MATH_H
#define SF_BIONIC_MATH_H
#include "android_abi.h"
#include "arm_scope.h"
#define SF_MATH_COUNT 48
SF_ANDROID_ABI float sf_math_sqrtf(float a);
SF_ANDROID_ABI float sf_math_floorf(float a);
SF_ANDROID_ABI float sf_math_ceilf(float a);
SF_ANDROID_ABI float sf_math_atanf(float a);
SF_ANDROID_ABI float sf_math_acosf(float a);
SF_ANDROID_ABI float sf_math_sinf(float a);
SF_ANDROID_ABI float sf_math_cosf(float a);
SF_ANDROID_ABI float sf_math_expf(float a);
SF_ANDROID_ABI float sf_math_tanf(float a);
SF_ANDROID_ABI float sf_math_asinf(float a);
SF_ANDROID_ABI float sf_math_roundf(float a);
SF_ANDROID_ABI float sf_math_log10f(float a);
SF_ANDROID_ABI float sf_math_rintf(float a);
SF_ANDROID_ABI float sf_math_truncf(float a);
SF_ANDROID_ABI float sf_math_exp2f(float a);
SF_ANDROID_ABI float sf_math_nearbyintf(float a);
SF_ANDROID_ABI float sf_math_powf(float a, float b);
SF_ANDROID_ABI float sf_math_fmodf(float a, float b);
SF_ANDROID_ABI float sf_math_remainderf(float a, float b);
SF_ANDROID_ABI float sf_math_atan2f(float a, float b);
SF_ANDROID_ABI float sf_math_fminf(float a, float b);
SF_ANDROID_ABI float sf_math_fmaxf(float a, float b);
SF_ANDROID_ABI double sf_math_floor(double a);
SF_ANDROID_ABI double sf_math_ceil(double a);
SF_ANDROID_ABI double sf_math_sqrt(double a);
SF_ANDROID_ABI double sf_math_sin(double a);
SF_ANDROID_ABI double sf_math_cos(double a);
SF_ANDROID_ABI double sf_math_tan(double a);
SF_ANDROID_ABI double sf_math_asin(double a);
SF_ANDROID_ABI double sf_math_acos(double a);
SF_ANDROID_ABI double sf_math_atan(double a);
SF_ANDROID_ABI double sf_math_exp(double a);
SF_ANDROID_ABI double sf_math_log(double a);
SF_ANDROID_ABI double sf_math_log10(double a);
SF_ANDROID_ABI double sf_math_round(double a);
SF_ANDROID_ABI double sf_math_trunc(double a);
SF_ANDROID_ABI double sf_math_nearbyint(double a);
SF_ANDROID_ABI double sf_math_pow(double a, double b);
SF_ANDROID_ABI double sf_math_fmod(double a, double b);
SF_ANDROID_ABI double sf_math_remainder(double a, double b);
SF_ANDROID_ABI double sf_math_atan2(double a, double b);
SF_ANDROID_ABI double sf_math_fmin(double a, double b);
SF_ANDROID_ABI double sf_math_fmax(double a, double b);
SF_ANDROID_ABI double sf_math_ldexp(double,int);
SF_ANDROID_ABI double sf_math_scalbn(double,int);
SF_ANDROID_ABI float sf_math_ldexpf(float,int);
SF_ANDROID_ABI double sf_math_modf(double,double *);
SF_ANDROID_ABI float sf_math_modff(float,float *);
int sf_bionic_math_catalog(ArmScopeEntry *,size_t);
#endif
