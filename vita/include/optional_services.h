#ifndef SF_OPTIONAL_H
#define SF_OPTIONAL_H
typedef enum { SERVICE_ANALYTICS, SERVICE_ADS, SERVICE_CLOUD, SERVICE_LICENSE, SERVICE_BILLING } Service;
typedef enum { SERVICE_UNAVAILABLE, SERVICE_REQUIRES_AUTHORIZED_BACKEND } ServiceResult;
typedef void (*ServiceCallback)(ServiceResult,void *);
/* Called on game thread; callback must not initiate another request recursively.
   Integration must post this call onto that thread, never invoke JNI from worker. */
static inline void optional_service_unavailable(Service s,ServiceCallback cb,void *ctx){
    ServiceResult result=(s==SERVICE_LICENSE||s==SERVICE_BILLING)?SERVICE_REQUIRES_AUTHORIZED_BACKEND:SERVICE_UNAVAILABLE;
    if(cb)cb(result,ctx);
}
#endif
