#include "touch.h"
#include <assert.h>
#include <stdio.h>
static int actions[32],counts[32],indexes[32],n;
static void record(const TouchEvent *e,void *ctx){(void)ctx;actions[n]=e->action;counts[n]=e->count;indexes[n++]=e->index;}
int main(void){
    TouchState s={0};
    touch_step(&s,B_CROSS,0,0,1,record,0);assert(n==1&&actions[0]==T_DOWN);
    touch_step(&s,B_CROSS,0,0,2,record,0);assert(n==1);
    touch_step(&s,B_CROSS|B_SQUARE,0,0,3,record,0);assert(actions[1]==T_POINTER_DOWN&&counts[1]==2&&indexes[1]==1);
    touch_step(&s,B_SQUARE,0,0,4,record,0);assert(actions[2]==T_POINTER_UP&&counts[2]==2&&indexes[2]==0);
    touch_step(&s,0,0,0,5,record,0);assert(actions[3]==T_UP&&s.count==0);
    touch_step(&s,0,0.8f,0,6,record,0);assert(s.count==1&&s.active[0].id==0);
    touch_step(&s,0,-0.8f,0,7,record,0);assert(actions[n-1]==T_MOVE);
    touch_cancel(&s,8,record,0);assert(actions[n-1]==T_CANCEL&&s.count==0);
    puts("touch sequencing PASS");return 0;
}
