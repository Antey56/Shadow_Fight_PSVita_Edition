#include "touch.h"
#include <math.h>
#include <string.h>
static void emit(TouchState *s,int action,int idx,uint64_t t,TouchSink sink,void *ctx){
    TouchEvent e={0}; e.action=action;e.index=idx;e.count=s->count;e.time_us=t;
    memcpy(e.points,s->active,sizeof(Pointer)*s->count);sink(&e,ctx);
}
void touch_cancel(TouchState *s,uint64_t t,TouchSink sink,void *ctx){
    if(s->count)emit(s,T_CANCEL,0,t,sink,ctx);
    s->count=0;
}
void touch_step(TouchState *s,unsigned b,float ax,float ay,uint64_t t,TouchSink sink,void *ctx){
    /* Example Android logical viewport 960x544. Coordinates MUST be calibrated. */
    Pointer desired[5];int n=0;
    if(b&(B_LEFT|B_RIGHT|B_UP|B_DOWN)){
        ax=((b&B_RIGHT)!=0)-((b&B_LEFT)!=0);ay=((b&B_DOWN)!=0)-((b&B_UP)!=0);
    }
    float length=sqrtf(ax*ax+ay*ay);
    if(length>0.20f){if(length>1){ax/=length;ay/=length;}desired[n++]=(Pointer){0,160+70*ax,420+70*ay};}
    unsigned keys[4]={B_CROSS,B_SQUARE,B_TRIANGLE,B_CIRCLE};
    float xy[4][2]={{840,450},{750,400},{840,350},{920,400}};
    for(int i=0;i<4;i++)if(b&keys[i])desired[n++]=(Pointer){i+1,xy[i][0],xy[i][1]};
    /* Release first, retaining old coordinates in POINTER_UP payload. */
    for(int i=s->count-1;i>=0;i--){
        int found=0;for(int j=0;j<n;j++)if(s->active[i].id==desired[j].id)found=1;
        if(!found){emit(s,s->count==1?T_UP:T_POINTER_UP,i,t,sink,ctx);
            memmove(s->active+i,s->active+i+1,sizeof(Pointer)*(s->count-i-1));s->count--;}
    }
    int moved=0;
    for(int j=0;j<n;j++){
        int i=0;while(i<s->count&&s->active[i].id!=desired[j].id)i++;
        if(i==s->count){s->active[s->count++]=desired[j];emit(s,s->count==1?T_DOWN:T_POINTER_DOWN,i,t,sink,ctx);}
        else if(s->active[i].x!=desired[j].x||s->active[i].y!=desired[j].y){s->active[i]=desired[j];moved=1;}
    }
    if(moved&&s->count)emit(s,T_MOVE,0,t,sink,ctx);
}
