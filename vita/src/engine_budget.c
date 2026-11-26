#include "engine_budget.h"
#include <limits.h>
int sf_engine_ram_threshold(uint32_t free_bytes,uint32_t pool,uint32_t keep,int *threshold){
    if(threshold)*threshold=0;
    if(!threshold||!pool||pool%4096||keep%4096||free_bytes>INT_MAX)return -120;
    if((uint64_t)pool+keep>free_bytes)return -121;
    *threshold=(int)(free_bytes-pool);return 0;
}
