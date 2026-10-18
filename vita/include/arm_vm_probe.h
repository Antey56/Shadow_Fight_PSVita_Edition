#ifndef SF_ARM_VM_PROBE_H
#define SF_ARM_VM_PROBE_H
#include <stdint.h>
#include <stddef.h>
typedef void (*SfArmProbeTrace)(const char *,int,uint32_t);
int sf_arm_vm_probe(int *,uint32_t *,float *,float *,char *,size_t,SfArmProbeTrace);
int sf_libmain_entry_probe(int *,uint32_t *,uint32_t *,uint32_t *,SfArmProbeTrace);
int sf_arm_vm_probe_shutdown(SfArmProbeTrace);
#endif
