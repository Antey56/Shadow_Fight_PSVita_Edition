#include "arm_reloc.h"
#include <stdlib.h>
typedef struct { uint32_t offset,value; } Write;
static uint32_t rd(const unsigned char *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void wr(unsigned char *p,uint32_t v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24);}
static int compare(const void *a,const void *b){
    uint32_t x=((const Write *)a)->offset,y=((const Write *)b)->offset;
    return (x>y)-(x<y);
}
int arm_reloc_apply(unsigned char *image,size_t size,uint32_t bias,const ArmReloc *rels,size_t count,size_t *failed){
    if(failed)*failed=(size_t)-1;
    if(!image||(!rels&&count)||count>SIZE_MAX/sizeof(Write))return -1;
    if(!count)return 0;
    Write *writes=malloc(count*sizeof(*writes));if(!writes)return -2;
    size_t used=0;int rc=0;
    for(size_t i=0;i<count;i++){
        const ArmReloc *r=&rels[i];uint32_t v=0;
        if(r->type==0)continue;
        if(r->type!=2&&r->type!=21&&r->type!=22&&r->type!=23){rc=-3;goto error;}
        if((r->offset&3)||r->offset>size||size-r->offset<4){rc=-4;goto error;}
        if(r->type!=23&&!r->symbol_resolved){rc=-5;goto error;}
        if(r->type==23){
            /* This API accepts only symbol-index-zero RELATIVE entries. */
            if(r->symbol_value||r->thumb){rc=-6;goto error;}
            v=bias+rd(image+r->offset);
        }else if(r->type==2){v=(r->symbol_value+rd(image+r->offset))|(r->thumb?1u:0u);}
        else if(r->type==21){v=(r->symbol_value+rd(image+r->offset))|(r->thumb?1u:0u);}
        else {v=r->symbol_value|(r->thumb?1u:0u);} /* REL JUMP_SLOT addend is 0 */
        writes[used++]=(Write){r->offset,v};
        continue;
error:
        if(failed)*failed=i;
        free(writes);return rc;
    }
    /* Duplicate places are ambiguous: reject rather than apply twice. */
    qsort(writes,used,sizeof(*writes),compare);
    for(size_t i=1;i<used;i++)if(writes[i-1].offset==writes[i].offset){free(writes);return -7;}
    for(size_t i=0;i<used;i++)wr(image+writes[i].offset,writes[i].value);
    free(writes);return 0;
}
