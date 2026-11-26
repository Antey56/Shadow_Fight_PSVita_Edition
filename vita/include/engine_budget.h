#ifndef SF_ENGINE_BUDGET_H
#define SF_ENGINE_BUDGET_H
#include <stdint.h>
/* Returns a vitaGL remaining-RAM threshold which caps the planned USER pool.
   keep_free includes pending native reservations plus explicit headroom.
   No allocations are performed and success does not prove engine peak fit. */
int sf_engine_ram_threshold(uint32_t,uint32_t,uint32_t,int *);
#endif
