#include "bionic_math.h"
#include <math.h>
#include <stdint.h>
SF_ANDROID_ABI float sf_math_sqrtf(float a){return sqrtf(a);}
SF_ANDROID_ABI float sf_math_floorf(float a){return floorf(a);}
SF_ANDROID_ABI float sf_math_ceilf(float a){return ceilf(a);}
SF_ANDROID_ABI float sf_math_atanf(float a){return atanf(a);}
SF_ANDROID_ABI float sf_math_acosf(float a){return acosf(a);}
SF_ANDROID_ABI float sf_math_sinf(float a){return sinf(a);}
SF_ANDROID_ABI float sf_math_cosf(float a){return cosf(a);}
SF_ANDROID_ABI float sf_math_expf(float a){return expf(a);}
SF_ANDROID_ABI float sf_math_tanf(float a){return tanf(a);}
SF_ANDROID_ABI float sf_math_asinf(float a){return asinf(a);}
SF_ANDROID_ABI float sf_math_roundf(float a){return roundf(a);}
SF_ANDROID_ABI float sf_math_log10f(float a){return log10f(a);}
SF_ANDROID_ABI float sf_math_rintf(float a){return rintf(a);}
SF_ANDROID_ABI float sf_math_truncf(float a){return truncf(a);}
SF_ANDROID_ABI float sf_math_exp2f(float a){return exp2f(a);}
SF_ANDROID_ABI float sf_math_nearbyintf(float a){return nearbyintf(a);}
SF_ANDROID_ABI float sf_math_powf(float a, float b){return powf(a,b);}
SF_ANDROID_ABI float sf_math_fmodf(float a, float b){return fmodf(a,b);}
SF_ANDROID_ABI float sf_math_remainderf(float a, float b){return remainderf(a,b);}
SF_ANDROID_ABI float sf_math_atan2f(float a, float b){return atan2f(a,b);}
SF_ANDROID_ABI float sf_math_fminf(float a, float b){return fminf(a,b);}
SF_ANDROID_ABI float sf_math_fmaxf(float a, float b){return fmaxf(a,b);}
SF_ANDROID_ABI double sf_math_floor(double a){return floor(a);}
SF_ANDROID_ABI double sf_math_ceil(double a){return ceil(a);}
SF_ANDROID_ABI double sf_math_sqrt(double a){return sqrt(a);}
SF_ANDROID_ABI double sf_math_sin(double a){return sin(a);}
SF_ANDROID_ABI double sf_math_cos(double a){return cos(a);}
SF_ANDROID_ABI double sf_math_tan(double a){return tan(a);}
SF_ANDROID_ABI double sf_math_asin(double a){return asin(a);}
SF_ANDROID_ABI double sf_math_acos(double a){return acos(a);}
SF_ANDROID_ABI double sf_math_atan(double a){return atan(a);}
SF_ANDROID_ABI double sf_math_exp(double a){return exp(a);}
SF_ANDROID_ABI double sf_math_log(double a){return log(a);}
SF_ANDROID_ABI double sf_math_log10(double a){return log10(a);}
SF_ANDROID_ABI double sf_math_round(double a){return round(a);}
SF_ANDROID_ABI double sf_math_trunc(double a){return trunc(a);}
SF_ANDROID_ABI double sf_math_nearbyint(double a){return nearbyint(a);}
SF_ANDROID_ABI double sf_math_pow(double a, double b){return pow(a,b);}
SF_ANDROID_ABI double sf_math_fmod(double a, double b){return fmod(a,b);}
SF_ANDROID_ABI double sf_math_remainder(double a, double b){return remainder(a,b);}
SF_ANDROID_ABI double sf_math_atan2(double a, double b){return atan2(a,b);}
SF_ANDROID_ABI double sf_math_fmin(double a, double b){return fmin(a,b);}
SF_ANDROID_ABI double sf_math_fmax(double a, double b){return fmax(a,b);}
SF_ANDROID_ABI double sf_math_ldexp(double a,int n){return ldexp(a,n);}
SF_ANDROID_ABI double sf_math_scalbn(double a,int n){return scalbn(a,n);}
SF_ANDROID_ABI float sf_math_ldexpf(float a,int n){return ldexpf(a,n);}
SF_ANDROID_ABI double sf_math_modf(double a,double *p){return modf(a,p);}
SF_ANDROID_ABI float sf_math_modff(float a,float *p){return modff(a,p);}
int sf_bionic_math_catalog(ArmScopeEntry *out,size_t cap){
#if defined(SF2_VITA_TARGET) && defined(__arm__)
    if(!out||cap<SF_MATH_COUNT||sizeof(uintptr_t)!=4)return -22;
#define E(n) (ArmScopeEntry){"host:bionic:math",#n,(uint32_t)(uintptr_t)&sf_math_##n,2,1}
    out[0]=E(sqrtf);
    out[1]=E(floorf);
    out[2]=E(ceilf);
    out[3]=E(atanf);
    out[4]=E(acosf);
    out[5]=E(sinf);
    out[6]=E(cosf);
    out[7]=E(expf);
    out[8]=E(tanf);
    out[9]=E(asinf);
    out[10]=E(roundf);
    out[11]=E(log10f);
    out[12]=E(rintf);
    out[13]=E(truncf);
    out[14]=E(exp2f);
    out[15]=E(nearbyintf);
    out[16]=E(powf);
    out[17]=E(fmodf);
    out[18]=E(remainderf);
    out[19]=E(atan2f);
    out[20]=E(fminf);
    out[21]=E(fmaxf);
    out[22]=E(floor);
    out[23]=E(ceil);
    out[24]=E(sqrt);
    out[25]=E(sin);
    out[26]=E(cos);
    out[27]=E(tan);
    out[28]=E(asin);
    out[29]=E(acos);
    out[30]=E(atan);
    out[31]=E(exp);
    out[32]=E(log);
    out[33]=E(log10);
    out[34]=E(round);
    out[35]=E(trunc);
    out[36]=E(nearbyint);
    out[37]=E(pow);
    out[38]=E(fmod);
    out[39]=E(remainder);
    out[40]=E(atan2);
    out[41]=E(fmin);
    out[42]=E(fmax);
    out[43]=E(ldexp);
    out[44]=E(scalbn);
    out[45]=E(ldexpf);
    out[46]=E(modf);
    out[47]=E(modff);
#undef E
    return SF_MATH_COUNT;
#else
    (void)out;(void)cap;return -22;
#endif
}
