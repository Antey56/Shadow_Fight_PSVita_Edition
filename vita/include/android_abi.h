#ifndef SF_ANDROID_ABI_H
#define SF_ANDROID_ABI_H
#if defined(__arm__) && defined(__GNUC__)
#define SF_ANDROID_ABI __attribute__((pcs("aapcs")))
#else
#define SF_ANDROID_ABI
#endif
#endif
