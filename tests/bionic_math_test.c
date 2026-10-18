#include "bionic_math.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void){
    assert(sf_math_sqrtf(9.0f)==3.0f);assert(sf_math_sqrt(16.0)==4.0);
    assert(sf_math_powf(2.0f,3.0f)==8.0f&&sf_math_pow(2.0,10.0)==1024.0);
    assert(sf_math_floorf(-1.25f)==-2.0f&&sf_math_ceil(-1.25)==-1.0);
    assert(sf_math_roundf(-1.5f)==-2.0f&&sf_math_truncf(-1.75f)==-1.0f);
    assert(sf_math_ldexp(1.25,4)==20.0&&sf_math_ldexpf(1.25f,-2)==0.3125f);
    assert(sf_math_scalbn(1.25,3)==10.0);
    assert(sf_math_fmodf(-5.5f,2.0f)==-1.5f&&sf_math_remainderf(5.5f,2.0f)==-0.5f);
    assert(sf_math_sinf(0)==0&&sf_math_cosf(0)==1&&sf_math_expf(0)==1);
    assert(sf_math_log10f(100)==2&&sf_math_exp2f(3)==8);
    float fi=0;double di=0;
    assert(sf_math_modff(-2.25f,&fi)==-0.25f&&fi==-2.0f);
    assert(sf_math_modf(3.5,&di)==0.5&&di==3.0);
    assert(isnan(sf_math_sqrtf(-1.0f)));assert(isinf(sf_math_pow(2.0,1024.0)));
    assert(signbit(sf_math_sqrtf(-0.0f)));
    assert(sf_math_fminf(NAN,4)==4&&sf_math_fmax(3,NAN)==3);
    assert(sf_math_atan2f(0,1)==0);
    assert(sf_bionic_math_catalog(0,0)==-22);
    puts("math adapter semantic cases PASS; target ABI is separate");return 0;
}
